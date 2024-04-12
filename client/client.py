import http.client
import http.client, urllib.parse
import random


def GETfunction(ip_puerto):
    #Conexion al proxy server
    conn = http.client.HTTPConnection(ip_puerto)
    conn.request("GET", "")
    r1 = conn.getresponse()
    print(r1.status, r1.reason)
    data1 = r1.read()  # Retorna todo el contenido
    conn.request("GET", "/")
    r1 = conn.getresponse()
    while chunk := r1.read(200):
        print(repr(chunk))

def POSTfunction(ip_puerto):
    params = urllib.parse.urlencode({'@number': 12524, '@type': 'issue', '@action': 'show'})
    headers = {"Content-type": "application/x-www-form-urlencoded",
               "Accept": "text/plain"}
    #Conexion al proxy server
    conn = http.client.HTTPConnection(ip_puerto)
    conn.request("POST", "", params, headers)
    response = conn.getresponse()
    print(response.status, response.reason)
    data = response.read()
    print(data)
    conn.close()

def HEADFunction(ip_puerto):
    conn = http.client.HTTPSConnection(ip_puerto)
    conn.request("HEAD", "/")
    res = conn.getresponse()
    print(res.status, res.reason)
    data = res.read()
    print(len(data))


def main():
    print('Digite la Ip y el puerto a usar (Ip:puerto): ')
    ip_puerto = input()  # Pedimos al usuario que ingrese el puerto
    num = random.randint(1, 3)

    if num == 1:
        GETfunction(ip_puerto)

    elif num == 2:
        POSTfunction(ip_puerto)

    elif num == 3:
        HEADFunction(ip_puerto)


if __name__ == "__main__":
    main()

