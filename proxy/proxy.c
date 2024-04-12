/*
 * Tiny TCP proxy server
 *
 * Author: Krzysztof Kliś <krzysztof.klis@gmail.com>
 * Fixes and improvements: Jérôme Poulin <jeromepoulin@gmail.com>
 * IPv6 support: 04/2019 Rafael Ferrari <rafaelbf@hotmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version with the following modification:
 *
 * As a special exception, the copyright holders of this library give you
 * permission to link this library with independent modules to produce an
 * executable, regardless of the license terms of these independent modules,
 * and to copy and distribute the resulting executable under terms of your choice,
 * provided that you also meet, for each linked independent module, the terms
 * and conditions of the license of that module. An independent module is a
 * module which is not derived from or based on this library. If you modify this
 * library, you may extend this exception to your version of the library, but
 * you are not obligated to do so. If you do not wish to do so, delete this
 * exception statement from your version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <resolv.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/wait.h>
#ifdef USE_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#define BUF_SIZE 16384

#define READ  0
#define WRITE 1

#define SERVER_SOCKET_ERROR     -1
#define SERVER_SETSOCKOPT_ERROR -2
#define SERVER_BIND_ERROR       -3
#define SERVER_LISTEN_ERROR     -4
#define CLIENT_SOCKET_ERROR     -5
#define CLIENT_RESOLVE_ERROR    -6
#define CLIENT_CONNECT_ERROR    -7
#define CREATE_PIPE_ERROR       -8
#define BROKEN_PIPE_ERROR       -9
#define SYNTAX_ERROR            -10
#define BACKLOG 20 //how many pending connections queue will hold

typedef enum {TRUE = 1, FALSE = 0} bool; //Para poder usar TRUE o FALSE en vez de 0 o 1

int check_ipversion(char * address); // Verifica si lo leido es ipv4 o ipv6
int create_socket(int port); //Abre el puerto del proxy server
void sigchld_handler(int signal);
void sigterm_handler(int signal);
void server_loop(); // Crea un proceso fork() que pone en modo de escucha al proxy
void handle_client(int client_sock, struct sockaddr_storage client_addr); //función que maneja las conexiones de los clientes
void forward_data(int source_sock, int destination_sock); /* recibe datos del cliente y los envia al  servidor http seleccionado por el round robin*/
void forward_data_ext(int source_sock, int destination_sock, char *cmd);/*recibe datos del servidor http y las envia al cliente*/
int create_connection(); //Crea todas las conexiones con todos los servidores http 
int parse_options(int argc, char *argv[]); /*maneja todas los datos de la entrada estándar*/
void plog(int priority, const char *format, ...);

int server_sock, client_sock, remote_sock, remote_port = 0; /*Los canales*/

int connections_processed = 0; //Cuantas conexiones ha atendido el proxy
char *bind_addr, *remote_host, *cmd_in, *cmd_out;
bool foreground = TRUE; //Flag para mandar el proceso a backgound, coloco FALSE,  el flag en TRUE permite ver la salida de los printf deñ proxy por consola
bool use_syslog = FALSE;
int numHosts=0; //Cuenta el número de servidores http van a interactuar con el proxy 
int numPorts=0; //Cuenta el número de puertos  donde se ejecuta cada servidor http 
struct arraysArr 
{
  char *remote_hostArr[5];//almacena las ips de los hosts http, este proxy se conecta máximo con 5 servidores http
  int  remote_portArr[5];//almacena los puertos de los hosts http
  int  numReqHost[5];   //almacena el número de solicitudes de los clientes atendidos por cada servidor http
}arr;
void bubble_sort(int n); /*Ordena de menor a mayor haciendo intercambios cuando los valores son iguales*/
void printArrays(int n); /*Imprime la manera RR como el proxy envía solicitudes a los servidores http


/* Program start */
int main(int argc, char *argv[]) {
    int local_port;
    pid_t pid;

    bind_addr = NULL;
    
    /*Inicializa en 0 el número de requerimientos que va atender cada servidor http*/
    for(int x=0; x<numHosts; x++)
        arr.numReqHost[x]=0;
    /*iPor defecto debe ser 8080 pero el usuarios es libre de colocarle cualquier valor*/
    local_port = parse_options(argc, argv);

    /* Verifica que el valor del puerto sea un número mayor o igual  a 0, muestra por consola como se debe ejecutar el proxy*/

    if (local_port < 0 ) {
        printf("Syntax: %s [-b bind_address] -l local_port -h <remote_host1,..,remote_hostN> -p <remote_port1,...,remote_portN> [-i \"input parser\"] [-o \"output parser\"] [-f (stay in foreground)] [-s (use syslog)]\n", argv[0]);
        return local_port;
    }

     /* Verifica que el número de ips sea igual al número de puertos  para que el proxy pueda crear 
      *  pueda crear las conexiones a cada servidor http en puertos ciertos
      */
    if(numPorts != numHosts)
    {
	printf("Syntax: %s [-b bind_address] -l local_port -h <remote_host1,..,remote_hostN> -p <remote_port1,...,remote_portN> [-i \"input parser\"] [-o \"output parser\"] [-f (stay in foreground)] [-s (use syslog)]\n", argv[0]);
	printf("numHost != numPorts\n");
	exit(0);
    }

    if (use_syslog) {
        openlog("proxy", LOG_PID, LOG_DAEMON);
    }
    /* Crea el canal (socket) donde va atender las solicitudes el proxy*/
    if ((server_sock = create_socket(local_port)) < 0) { // start server
        plog(LOG_CRIT, "Cannot run server: %m");
        return server_sock;
    }

    signal(SIGCHLD, sigchld_handler); // prevent ended children from becoming zombies
    signal(SIGTERM, sigterm_handler); // handle KILL signal
    
    if (foreground) {
        server_loop(); /* ejecuta el servidor proxy en  primer plano(foreground) */
    } else {
        switch(pid = fork()) {
        //Ejecuta el servidor iproxy en backgound como un proceso hijo daemon a tra    vés del fork()
            case 0: // deamonized child
                server_loop();
                break;
            case -1: // error
                plog(LOG_CRIT, "Cannot daemonize: %m"); // si el PID = -1 no puede ejecutarse en background
                return pid;
            default: // parent
                close(server_sock); //cierra el canal que abrio en el puerto 8080
        }
    }

    if (use_syslog) {
        closelog();
    }

    return EXIT_SUCCESS;
}

/* Parse command line options */
int parse_options(int argc, char *argv[]) {
    int c, local_port = 0;

    while ((c = getopt(argc, argv, "b:l:h:p:i:o:fs")) != -1) {
        switch(c) {
            case 'l':
		//Captura de los argv despues del -l el puerto donde el usuario define que va a escuchar peticiones el proxy
                local_port = atoi(optarg);
                break;
            case 'b':
                bind_addr = optarg;
                break;
	    //procesa la lista de host que vienen separadas por , despues del -h
            case 'h':
		arr.remote_hostArr[numHosts++] = strtok(optarg, ",");
		while( (arr.remote_hostArr[numHosts] = strtok(NULL, ","))!=NULL)
                {
		   numHosts++;
		}
		remote_host = arr.remote_hostArr[0];
		for(int x=0; x<numHosts; x++)
			printf("Hosts:%s\t", arr.remote_hostArr[x]);
		printf("\nnumH=%d\n",  numHosts);
                break;
	    //procesa la lista de puertos que vienen separadas por , despues del -p
            case 'p':
                char *stringtemp =  strtok(optarg, ",");
		arr.remote_portArr[numPorts++] = atoi(stringtemp);
//		while((strcpy(arr.remote_portArr[numPorts],strtok(NULL, ",")))!=NULL)
		while((stringtemp= strtok(NULL, ",")) != NULL)
                {
		   arr.remote_portArr[numPorts]= atoi(stringtemp);
		   numPorts++;
		}
		remote_port= arr.remote_portArr[0];
		for(int x=0; x<numPorts; x++)
			printf("ports:%d\n", arr.remote_portArr[x]);
		printf("numPOR=%d\n", numPorts);
                break;
	    //procesa el valor después del -i y crea un archivo con ese nombre, que almacena las solicitudes de datos de los clientes 
            case 'i':
                cmd_in = optarg;
                break;
	    //procesa el valor después del -i y crea un archivo con ese nombre, que almacena las respuestas  de los servidores http a los  clientes 
            case 'o':
                cmd_out = optarg;
                break;
            case 'f':
                foreground = FALSE; // ejecuta por defecto el proxy server en background
                break;
            case 's':
                use_syslog = TRUE;
                break;
        }
    }

    if (local_port && remote_host && remote_port) {
        return local_port;
    } else {
        return SYNTAX_ERROR;
    }
}

int check_ipversion(char * address)
{
/* Check for valid IPv4 or Iv6 string. Returns AF_INET for IPv4, AF_INET6 for IPv6 */
/* The InetPton function converts an IPv4 or IPv6 Internet network address in its standard 
 * text presentation form into its numeric binary form. The ANSI version of this function is inet_pton.
 * https://learn.microsoft.com/en-us/windows/win32/api/ws2tcpip/nf-ws2tcpip-inet_pton
 */	

    struct in6_addr bindaddr;

    if (inet_pton(AF_INET, address, &bindaddr) == 1) {
         return AF_INET;
    } else {
        if (inet_pton(AF_INET6, address, &bindaddr) == 1) {
            return AF_INET6;
        }
    }
    return 0;
}

/* Create server socket */
int create_socket(int port) {
    int server_sock, optval = 1;
    int validfamily=0;
    struct addrinfo hints, *res=NULL;
    char portstr[12];

    memset(&hints, 0x00, sizeof(hints));
    server_sock = -1;
    /* https://man7.org/linux/man-pages/man3/getaddrinfo.3.html */
    hints.ai_flags    = AI_NUMERICSERV;   /* numeric service number, not resolve */
    hints.ai_family = AF_UNSPEC;   /*address family for the returned addresses*/
    hints.ai_socktype = SOCK_STREAM; /*This field specifies the preferred socket type, for example SOCK_STREAM or SOCK_DGRAM. For this program proxy server use SOCK_STREAM, TCP socket connection oriented*/

    /* prepare to bind on specified numeric address */
    if (bind_addr != NULL) {
        /* check for numeric IP to specify IPv6 or IPv4 socket */
        if (validfamily = check_ipversion(bind_addr)) {
             hints.ai_family = validfamily;
             hints.ai_flags |= AI_NUMERICHOST; /* bind_addr is a valid numeric ip, skip resolve */
        }
    } else {
        /* if bind_address is NULL, will bind to IPv6 wildcard */
        hints.ai_family = AF_INET6; /* Specify IPv6 socket, also allow ipv4 clients */
        hints.ai_flags |= AI_PASSIVE; /* Wildcard address */
    }

    sprintf(portstr, "%d", port);

    /* Check if specified socket is valid. Try to resolve address if bind_address is a hostname */
    if (getaddrinfo(bind_addr, portstr, &hints, &res) != 0) {
        return CLIENT_RESOLVE_ERROR;
    }

    /*create a server socket*/
    if ((server_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        return SERVER_SOCKET_ERROR;
    }


    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        return SERVER_SETSOCKOPT_ERROR;
    }
    /* assigns the address specified by addr to the socket referred to
       by the file descriptor sockfd.  addrlen specifies the size, in
       bytes, of the address structure pointed to by addr.
       Traditionally, this operation is called “assigning a name to a
       socket”. https://man7.org/linux/man-pages/man2/bind.2.html*/
    if (bind(server_sock, res->ai_addr, res->ai_addrlen) == -1) {
            close(server_sock);
        return SERVER_BIND_ERROR;
    }
    /*marks the socket referred to by sockfd as a passive
       socket, that is, as a socket that will be used to accept incoming
       connection requests. https://man7.org/linux/man-pages/man2/listen.2.html */
    if (listen(server_sock, BACKLOG) < 0) {
        return SERVER_LISTEN_ERROR;
    }

    if (res != NULL) {
        freeaddrinfo(res);
    }

    return server_sock;
}

/* Send log message to stderr or syslog */
/* Escribe a través del stderr los errores del programa a archivos log del sistema operativo linux*/
void plog(int priority, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);

    if (use_syslog) {
        vsyslog(priority, format, ap);
    } else {
        vfprintf(stderr, format, ap);
        fprintf(stderr, "\n");
    }

    va_end(ap);
}

/* Update systemd status with connection count */
void update_connection_count()
{
#ifdef USE_SYSTEMD
    sd_notifyf(0, "STATUS=Ready. %d connections processed.\n", connections_processed);
#endif
}

/* Handle finished child process */
void sigchld_handler(int signal) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* Handle term signal */
void sigterm_handler(int signal) {
    close(client_sock);
    close(server_sock);
    exit(0);
}

/* Main server loop */
void server_loop() {
    struct sockaddr_storage client_addr;
    socklen_t addrlen = sizeof(client_addr);

#ifdef USE_SYSTEMD
    sd_notify(0, "READY=1\n");
#endif
    while (TRUE) {
       update_connection_count();
	/*It extracts the first
          connection request on the queue of pending connections for the
          listening socket, sockfd, creates a new connected socket, and
          returns a new file descriptor referring to that socket.
	  https://man7.org/linux/man-pages/man2/accept.2.html
       */
	/*acepta solicitudes de los canales de los clientes*/
       client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addrlen);

       /* Las siguientes líneas definen cual servidor http va atender al cliente
	* bubble_sort ordena el arreglo arr de menor a mayor basado en el número de peticiones 
	* atendida por cada servidor http. En la posición 0 queda siempre el servidor con menos
	* solicitudes atendidas. Garantizando el Round Robin equitativo tratando todos los servidores
	* con la misma prioridad. Cuando se le asigna a un servidor una peticion se incrementa en 1 su campo
	* numReqHost - numero de solicitudes a ese hosts
	* De los arreglos se almacena en las variables remote_port y remote_host 
	* la ip y el puerto deli http server  con menores clientes atendidos 
	*/
       int x=0;
       bubble_sort(numHosts); 
       remote_port=arr.remote_portArr[x]; 
       remote_host=arr.remote_hostArr[x];

       printf("Hosts= %s\t", arr.remote_hostArr[x]);
       printf("port= %d \t", arr.remote_portArr[x]);

       printf("sume \n");
       arr.numReqHost[x]=arr.numReqHost[x]+1;
       printf("req= %d \n", arr.numReqHost[x]);
       printf("Function handle_client\n");
       printArrays(numHosts);


        if (fork() == 0) { // handle client connection in a separate process
            close(server_sock);
	    /*se maneja la conexion con los clientesi, la función fork crea un un proceso nuevo hijo independiente
	     *para atender independiente mente cada cliente, con el canal de dirección  del cliente.*/
            handle_client(client_sock, client_addr);
            exit(0);
        } else {
            connections_processed++;
	    printf("Conexiones procesadas por el proxy=%d\n",connections_processed);
        }
        close(client_sock);
    }

}


/* Handle client connection */
void handle_client(int client_sock, struct sockaddr_storage client_addr)
{


       if ((remote_sock = create_connection(remote_port, remote_host)) < 0) {
              plog(LOG_ERR, "Cannot connect to http host: %m");
              goto cleanup;
       }

    if (fork() == 0) { // a process forwarding data from client to remote socket
        if (cmd_out) {
            forward_data_ext(client_sock, remote_sock, cmd_out);
        } else {
            forward_data(client_sock, remote_sock);
        }
        exit(0);
    }

    if (fork() == 0) { // a process forwarding data from remote socket to client
        if (cmd_in) {
            forward_data_ext(remote_sock, client_sock, cmd_in);
        } else {
            forward_data(remote_sock, client_sock);
        }
        exit(0);
    }

cleanup:
    close(remote_sock);
    close(client_sock);
}

/* Forward data between sockets */
void forward_data(int source_sock, int destination_sock) {
    ssize_t n;

#ifdef USE_SPLICE
    int buf_pipe[2];

    if (pipe(buf_pipe) == -1) {
        plog(LOG_ERR, "pipe: %m");
        exit(CREATE_PIPE_ERROR);
    }

    while ((n = splice(source_sock, NULL, buf_pipe[WRITE], NULL, SSIZE_MAX, SPLICE_F_NONBLOCK|SPLICE_F_MOVE)) > 0) {
        if (splice(buf_pipe[READ], NULL, destination_sock, NULL, SSIZE_MAX, SPLICE_F_MOVE) < 0) {
            plog(LOG_ERR, "write: %m");
            exit(BROKEN_PIPE_ERROR);
        }
    }
#else
    char buffer[BUF_SIZE];

    while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) { // read data from input socket
        send(destination_sock, buffer, n, 0); // send data to output socket
    }
#endif

    if (n < 0) {
        plog(LOG_ERR, "read: %m");
        exit(BROKEN_PIPE_ERROR);
    }

#ifdef USE_SPLICE
    close(buf_pipe[0]);
    close(buf_pipe[1]);
#endif

    shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
    close(destination_sock);

    shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
    close(source_sock);
}

/* Forward data between sockets through external command */
void forward_data_ext(int source_sock, int destination_sock, char *cmd) {
    char buffer[BUF_SIZE];
    int n, i, pipe_in[2], pipe_out[2];

    if (pipe(pipe_in) < 0 || pipe(pipe_out) < 0) { // create command input and output pipes
        plog(LOG_CRIT, "Cannot create pipe: %m");
        exit(CREATE_PIPE_ERROR);
    }

    if (fork() == 0) {
        dup2(pipe_in[READ], STDIN_FILENO); // replace standard input with input part of pipe_in
        dup2(pipe_out[WRITE], STDOUT_FILENO); // replace standard output with output part of pipe_out
        close(pipe_in[WRITE]); // close unused end of pipe_in
        close(pipe_out[READ]); // close unused end of pipe_out
        n = system(cmd); // execute command
        exit(n);
    } else {
        close(pipe_in[READ]); // no need to read from input pipe here
        close(pipe_out[WRITE]); // no need to write to output pipe here

        while ((n = recv(source_sock, buffer, BUF_SIZE, 0)) > 0) { // read data from input socket
            if (write(pipe_in[WRITE], buffer, n) < 0) { // write data to input pipe of external command
                plog(LOG_ERR, "Cannot write to pipe: %m");
                exit(BROKEN_PIPE_ERROR);
            }
            if ((i = read(pipe_out[READ], buffer, BUF_SIZE)) > 0) { // read command output
                send(destination_sock, buffer, i, 0); // send data to output socket
            }
        }

        shutdown(destination_sock, SHUT_RDWR); // stop other processes from using socket
        close(destination_sock);

        shutdown(source_sock, SHUT_RDWR); // stop other processes from using socket
        close(source_sock);
    }
}

/* Create  todas las http Server  connections, dependiendo de lo que envie la función
 * handle_client con remote_port, remote_host,  la función handle_client, haciendo  que se */ 
//int create_connection() { original
int create_connection(int remote_port, char *remote_host) {
    struct addrinfo hints, *res=NULL;
    int sock;
    int validfamily=0;
    char portstr[12];

    memset(&hints, 0x00, sizeof(hints));

    hints.ai_flags    = AI_NUMERICSERV; /* numeric service number, not resolve */
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    sprintf(portstr, "%d", remote_port);
    printf(" remote port %d\n", remote_port);
    printf(" remote host %s\n", remote_host);
    printf(" portstr %s\n", portstr);


    /* check for numeric IP to specify IPv6 or IPv4 socket */
    if ((validfamily = check_ipversion(remote_host))) {
         hints.ai_family = validfamily;
         hints.ai_flags |= AI_NUMERICHOST;  /* remote_host is a valid numeric ip, skip resolve */
    }

    /* Check if specified host is valid. Try to resolve address if remote_host is a hostname */
    if (getaddrinfo(remote_host,portstr , &hints, &res) != 0) {
        errno = EFAULT;
        return CLIENT_RESOLVE_ERROR;
    }

    if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
        return CLIENT_SOCKET_ERROR;
    }

    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        return CLIENT_CONNECT_ERROR;
    }

    if (res != NULL) {
        freeaddrinfo(res);
    }

    return sock;
}

void bubble_sort(int n)
{
    /* al hacer burbuja intercambiando los valores que son == se grantiza el round robin equitativo
     * y en la línea 346 de la función server_loop(), se toma la dirección ip y el puerto del servidor
     * que tiene el valor más bajo, asi se van corriendo los servidores hasta hacer que el primero que
     * atendio, no se vuelva a usar hasta que los demas hallan realizado atenciones a los clientes.
     * */
    int j, k, rq, pp;
    char *hh;

    for (j = 0; j < (n - 1); j++) {
        for (k = 0; k < n - j - 1; k++) {
            if (arr.numReqHost[k] >= arr.numReqHost[k + 1]) {

                /* Swapping */
                rq =  arr.numReqHost[k];
                pp = arr.remote_portArr[k];
                hh = arr.remote_hostArr[k];

                arr.numReqHost[k] = arr.numReqHost[k + 1];
                arr.remote_portArr[k] = arr.remote_portArr[k + 1];
		arr.remote_hostArr[k] = arr.remote_hostArr[k + 1];

                arr.numReqHost[k + 1] = rq;
		arr.remote_portArr[k + 1] = pp;
		arr.remote_hostArr[k + 1] = hh;
            }
	}
    }	
    printf("Arreglo Ordenado por burbuja\n");
    printArrays(n);
    
}

void printArrays(int n){
    /*
     * La función muestra como el proxy envia solicitudes a los servidores http
     * e imprime las ip del servidor http que atendio la solicitud y el número de 
     * clientes atendidos
     */	
    int j=0;	
    for (j = 0; j < n ; j++) {
	    printf("http host ip= %s numReq Served= %d\n", arr.remote_hostArr[j], arr.numReqHost[j]);
    }	    
}

