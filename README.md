# README PROYECTO 1 TELEMATICA

## Video Explicacion en YouTube
<https://youtu.be/URIkfOqyFmE>

## Introducción

El proyecto se centró en la implementación de un cliente, 3 servidores basados en AWS usando Ubuntu y un proxy el cual implementaba un balanceador de cargas con el método Round Robin. Se usó el protocolo HTTP/1.1 y la API de Berkeley para manejar las distintas conexiones y las solicitudes HTTP, donde se vieron aspectos relacionados con la concurrencia, con el fin de mejorar el rendimiento del servidor y del proxy.

## Desarrollo

El desarrollo del proyecto implicó varias etapas:

1. Implementación del Servidor HTTP/1.1: Se desarrolló el server para manejar las solicitudes GET, POST Y HEAD (El cual más adelante hablamos de las dificultades), así como las respuestas que tenían que ver con el protocolo HTTP/1.1. Se usó además la API para manejar las conexiones entrantes y salientes, lo que permitió al servidor comunicarse con los clientes correctamente.

2. Creación del cliente en Python: Se creó el cliente en Python (Por la facilidad de conexión a C, dado que es un lenguaje basado en C) el cual se encarga de enviar solicitudes GET, POST y HEAD al server. Esto se hizo usando el módulo "http.client", el cual forma parte de la API manejando las conexiones y las solicitudes.

3. Desarrollo del proxy: Se implementó un proxy para actuar como intermediario entre el cliente y el servidor. Además, se agregó un balanceador de carga al proxy utilizando el algoritmo de ordenamiento de burbuja como método de round robin. La concurrencia se utilizó para manejar múltiples conexiones entrantes y salientes de manera simultánea, lo que mejoró el rendimiento del proxy al distribuir la carga de manera equitativa entre los servidores backend.

En resumen, en el proyecto se implementaron redes concurrentes utilizando la API de Berkeley para manejar las conexiones y las solicitudes HTTP, lo que permitió desarrollar varios servidores, un cliente y un proxy capaces de comunicarse de manera efectiva a través del protocolo HTTP/1.1.

### Implementación para el desarrollo

*sudo /etc/init.d/apache2 stop*   -->  Detiene el servidor Apache

*ps -fea|grep [http o proxy]*  --> Ver los procesos que están corriendo en background

*sudo kill -9 [# de proceso]* --> Se usa para detener los procesos 

*./proxy -l 8080 -h 172.31.32.101,172.31.41.96,172.31.34.45 -p 80,80,80 -i "tee input.log" -o "tee output.log"*   --> Ejecuta proxy con los logs

*rm *.log*  --> Borra todos los archivos .log

*cat output.log|more*    -->  Ver los logs de output

*cat input.log|more*    -->  Ver los logs de input

*gcc httpd.c -o httpServer* --> Compilar los archivos en C

*python3 cliente.py  [ip separada con ':' y puerto ip:puerto]* --> Ejecuta el cliente en Python

## Aspectos Logrados y No logrados

### Logrados:
- Implementación funcional del cliente en Python.
- Desarrollo del servidor para manejar solicitudes GET y POST.
- Creación de un proxy funcional con balanceador de carga.

### No logrados:

- La funcionalidad HEAD, no pudimos implementarla debido a las dificultades para depurar el flujo de los datos que se recibían al hacer la solicitud.

- Al principio, los problemas con la gestión de la memoria en el server, causaron problemas y errores debido a la "basura" que se quedaba al final, sin limpiarse luego de las solicitudes.

- El contador interno del proxy para realizar el balanceo de las cargas entre los servidores, no funcionaba bien, dado que se encontraba en un bloque de "while" incorrecto, el cual reiniciaba el contador con cada solicitud.

- Los problemas ocasionales al usar Apache, dado que se quedaba el puerto deseado abierto, el cual necesitaba ser detenido de forma manual.

## Conclusiones

- En cuanto al lenguaje de programación, la implementación en diferentes lenguajes de programación, no afecto realmente el funcionamiento final del sistema, a pesar de funcionar mejor en algunos que en otros, la comunicación al final se basa en un conjunto de parámetros estándar.

-La comunicación entre cliente/servidor, a través de puertos requiere que los dos estén "activos y escuchando" para poder enviar y recibir solicitudes.

- La implementación del proyecto nos demuestra la importancia de la lógica y el software en las redes actuales, donde se demuestra que con pocos componentes físicos involucrados es posible realizar una red compleja en comparación con otros sistemas.

## Referencias

A very simple HTTP server in C, for Unix, using fork(). (s. f.). Gist. https://gist.github.com/laobubu/d6d0e9beb934b60b2e552c2d03e1409e 

Http.client — cliente de protocolo HTTP. (s. f.). Python Documentation. https://docs.python.org/es/3/library/http.client.html 

Http.client — HTTP protocol client. (s. f.). Python Documentation. https://docs.python.org/3/library/http.client.html#examples 

Kklis. (s. f.). proxy/proxy.c at master · kklis/proxy. GitHub. https://github.com/kklis/proxy/blob/master/proxy.c
