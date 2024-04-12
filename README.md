# HTTP-Proxy-Telematica-Proyecto-1

## Introducción

Este proyecto se centró en la implementación de un cliente, 3 servidores basados en AWS usando Ubuntu y un proxy utilizando el protocolo HTTP/1.1 y balanceador de cargas implementando el metodo Round Robin. Inicialmente, se intentó desarrollar el cliente en Java, pero debido a dificultades, se optó por utilizar Python. Se abordaron aspectos como el funcionamiento de las solicitudes GET y POST, así como la implementación de un balanceador de carga en el proxy utilizando el algoritmo de ordenamiento de burbuja.

## Desarrollo

El desarrollo del proyecto implicó varias etapas:

1. **Implementación del servidor HTTP/1.1**: Se desarrolló el servidor para manejar las solicitudes GET y POST, así como las respuestas correspondientes de acuerdo con el protocolo HTTP/1.1.

2. **Creación del cliente en Python**: Se creó un cliente en Python para enviar solicitudes GET y POST al servidor. Esto se hizo después de intentar sin éxito implementar el cliente en Java.

3. **Desarrollo del proxy**: Se implementó un proxy para actuar como intermediario entre el cliente y el servidor. Además, se agregó un balanceador de carga al proxy utilizando el algoritmo de ordenamiento de burbuja como método de round-robin.

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
- La funcionalidad HEAD no pudo ser implementada debido a dificultades para depurar el flujo de datos recibido.
- Al principio, problemas con la gestión de la memoria en el servidor causaron errores debido a la basura dejada sin limpiar después de las solicitudes.
- El contador interno del proxy para realizar el balanceo de carga no funcionaba correctamente dentro de un bucle que reiniciaba el contador con cada solicitud.
- Problemas ocasionales al usar Apache, donde a veces se quedaba corriendo en el puerto deseado, requiriendo la terminación manual del proceso para liberar el puerto.

## Conclusiones

- A nivel de lenguaje de programación, la implementación en diferentes lenguajes no afectó significativamente el funcionamiento final del sistema, ya que la comunicación se basa en un conjunto de parámetros estándar.
- La comunicación entre el cliente y el servidor a través de puertos requiere que ambos estén activos y "escuchando" para enviar y recibir solicitudes.
- La implementación de este proyecto resalta la importancia de la lógica y el software en las redes, con pocos componentes físicos involucrados en comparación con otros sistemas. 

## Referencias

A very simple HTTP server in C, for Unix, using fork(). (s. f.). Gist. https://gist.github.com/laobubu/d6d0e9beb934b60b2e552c2d03e1409e 

Http.client — cliente de protocolo HTTP. (s. f.). Python Documentation. https://docs.python.org/es/3/library/http.client.html 

Http.client — HTTP protocol client. (s. f.). Python Documentation. https://docs.python.org/3/library/http.client.html#examples 

Kklis. (s. f.). proxy/proxy.c at master · kklis/proxy. GitHub. https://github.com/kklis/proxy/blob/master/proxy.c
