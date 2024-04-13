#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>
#define CONNMAX 1000
#ifndef _HTTPD_H___
#define _HTTPD_H___
#include <string.h>
#include <stdio.h>


//Server control functions


// Client request

char    *method,    // "GET" or "POST" or "HEAD"
        *uri,       // "/index.html" things before '?'
        *qs,        // "a=1&b=2"     things after  '?'
        *prot;      // "HTTP/1.1"

char    *payload;     // for POST
int      payload_size;

void serve_forever(const char *PORT);
char *request_header(const char* name);
// user shall implement this function
// URI Unified Resource Identifier es el path donde esta el archivo de inicio de http index.html
void route();

// some interesting macro for `route()`
#define ROUTE_START()       if (0) {
#define ROUTE(METHOD,URI)   } else if (strcmp(URI,uri)==0&&strcmp(METHOD,method)==0) {
#define ROUTE_GET(URI)      ROUTE("GET", URI)
#define ROUTE_POST(URI)     ROUTE("POST", URI)
#define ROUTE_HEAD(URI)     ROUTE("HEAD",URI)
#define ROUTE_END()         } else printf(\
                                "HTTP/1.1 500 Not Handled\r\n\r\n" \
                                "The server has no handler to the request.\r\n" \
                            );

#endif

static int listenfd, clients[CONNMAX]; //identificadores
static void error(char *); //funcion que imprime si hay errores
static void startServer(const char *); //Inicializa el servidor
static void respond(int); //Responde a los canales de los clientes

typedef struct { char *name, *value; } header_t;
static header_t reqhdr[17] = { {"\0", "\0"} };
static int clientfd; //el servidor diferencia que cllente esta haciendo la peticion

static char *buf; //Buffer para Almacenar Los STREAM que vienen de los clientes

void serve_forever(const char *PORT)
{
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char c;    
    
    int slot=0;
    
    printf(
            "Server started %shttp://127.0.0.1:%s%s\n",
            "\033[92m",PORT,"\033[0m"
            );

    // Setting all elements to -1: signifies there is no client connected
    int i;
    for (i=0; i<CONNMAX; i++)
        clients[i]=-1;
    startServer(PORT);
    
    // Ignore SIGCHLD to avoid zombie threads
    signal(SIGCHLD,SIG_IGN);

    // ACCEPT connections
    while (1) //Acepta y recibe paquetes de las solicitudes (request) clientes
    {
        addrlen = sizeof(clientaddr);
        clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

        if (clients[slot]<0)
        {
            perror("accept() error");
        }
        else
        {
            if ( fork()==0 ) //Funcion de C que clona todo este proceso
            {
                respond(slot);
                exit(0);
            }
        }

        while (clients[slot]!=-1) slot = (slot+1)%CONNMAX;
    }
}

//start server
void startServer(const char *port)
{
    struct addrinfo hints, *res, *p;

    // getaddrinfo for host
    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  //protocolos del stack TCP/IP (TCP, UDP, IPV4,IPV6)
    hints.ai_socktype = SOCK_STREAM; //Se comunica con TCP
    hints.ai_flags = AI_PASSIVE; //http://ecos.sourceware.org/docs-2.0/ref/net-common-tcpip-manpages-getaddrinfo.html
    if (getaddrinfo( NULL, port, &hints, &res) != 0)
    {
        perror ("getaddrinfo() error");
        exit(1);
    }
    // socket and bind
    for (p = res; p!=NULL; p=p->ai_next)
    {
        int option = 1;
        listenfd = socket (p->ai_family, p->ai_socktype, 0);
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)); //crear el canal con TCP
        if (listenfd == -1) continue;
        if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) break; //Une al puerto con el protocolo
    }
    if (p==NULL)
    {
        perror ("socket() or bind()");
        exit(1);
    }

    freeaddrinfo(res);

    // listen for incoming connections
    if ( listen (listenfd, 1000000) != 0 )
    {
        perror("listen() error");
        exit(1);
    }
}


// get request header
char *request_header(const char* name)
{
    header_t *h = reqhdr;
    while(h->name) {
        if (strcmp(h->name, name) == 0) 
        {

		return h->value;
        }
        h++;
    }
    return NULL;
}

char *sendHeader(char *url)
{
	size_t	len;
	char	*ct;

	len = strlen(url);
	if (!strcmp(&url[len-4], ".gif"))
		ct = "image/gif";
	else if (!strcmp(&url[len-5], ".html"))
		ct = "text/html";
	else
		ct = "text/plain";

	printf("Content-Type: %s\n", ct);

	return(ct);
}
//client connection
void respond(int n)
{
    int rcvd, fd, bytes_read;
    char *ptr;

    buf = malloc(65535);
    rcvd=recv(clients[n], buf, 65535, 0);

    if (rcvd<0)    // receive error
        fprintf(stderr,("recv() error\n"));
    else if (rcvd==0)    // receive socket closed
        fprintf(stderr,"Client disconnected upexpectedly.\n");
    else    // message received
    {
        buf[rcvd] = '\0';
	printf("\n\nbuf= %d,|%s|", rcvd,buf);
	printf("\n\nLORE\n\n ");

        method = strtok(buf,  " \t\r\n");
        uri    = strtok(NULL, " \t");
        prot   = strtok(NULL, " \t\r\n"); 

        fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);
        fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);
        fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, prot);
        
        if (qs = strchr(uri, '?'))
        {
            *qs++ = '\0'; //split URI
        } else {
            qs = uri - 1; //use an empty string
        }

        header_t *h = reqhdr;
        char *t, *t2;
        while(h < reqhdr+16) {
            char *k,*v,*t;
            k = strtok(NULL, "\r\n: \t"); if (!k) break;
            v = strtok(NULL, "\r\n");     while(*v && *v==' ') v++;
            h->name  = k;
            h->value = v;
            h++;
            fprintf(stderr, "[H] %s: %s\n", k, v);
            t = v + 1 + strlen(v);
            if (t[1] == '\r' && t[2] == '\n') break;
        }
        t++; // now the *t shall be the beginning of user payload
        t2 = request_header("Content-Length"); // and the related header if there is  
        payload = t;
        payload_size = t2 ? atol(t2) : (rcvd-(t-buf));

        // bind clientfd to stdout, making it easier to write
        clientfd = clients[n];
        dup2(clientfd, STDOUT_FILENO); //escribe al socket lo que armo en el fprint
        close(clientfd);

        // call router
        route();

        // tidy up
        fflush(stdout);
        shutdown(STDOUT_FILENO, SHUT_WR);
        close(STDOUT_FILENO);
    }

    //Closing SOCKET
    shutdown(clientfd, SHUT_RDWR);         //All further send and recieve operations are DISABLED...
    close(clientfd);
    clients[n]=-1;
}

/* argumentos vector de cadenas son argumentos de la entrada estandar, args[0] program name */
/* args[0] y args[1]*/

int main(int c, char** v)
{
    serve_forever(v[1]);
    return 0;
}

void route()
{
    ROUTE_START()

    ROUTE_GET("/")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Hola! Estas usando %s", request_header("User-Agent"));
        printf("%s", uri);
    }

    ROUTE_POST("/")
    {
        printf("HTTP/1.1 200 OK\r\n\r\n");
        printf("Haz posteado %d bytes. \r\n", payload_size);
        printf("Fetch los datos usando la variable 'payload'");
    }

    ROUTE_HEAD("/")
    {
	printf("HTTP/1.1 200 OK\r\n");
	printf("Tipo de contenido: %s\r\n", sendHeader(uri));
	printf("Conexion: cerrada\r\n");
        //printf("%s", uri);
	printf("\r\n");
    }
  
    ROUTE_END()
}
