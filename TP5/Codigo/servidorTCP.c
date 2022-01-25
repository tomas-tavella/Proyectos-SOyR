#include <stdio.h>
#include <string.h>
#include <signal.h> 
#include <sys/wait.h>

#include <sys/types.h>    
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>


//----- Defines -------------------------------------------------------------
#define  PORT_NUM 1050  // Numero de Port
#define  IP_ADDR "127.0.0.1" // Direccion IP LOCALHOST
#define NCONCUR 5
#define SOCKET_PROTOCOL 0

/*-----------------Global---------------*/
int terminar=0;

void handler(int sig);

//===== Main program ========================================================
int main(int argc, char *argv[]) {

    unsigned int         server_s;        // Descriptor del socket
    unsigned int         connect_s;       // Connection socket descriptor
    struct sockaddr_in   server_addr;     // Estructura con los datos del servidor
    struct sockaddr_in   client_addr;     // Estructura con los datos del cliente
    struct in_addr       client_ip_addr;  // Client IP address
    int                  addr_len;        // Tamaño de las estructuras
    char                 buf_tx[1500];    // Buffer de 1500 bytes para los datos a transmitir
    char                 buf_rx[1500];    // Buffer de 1500 bytes para los datos a transmitir
    int                  bytesrecibidos, bytesaenviar, bytestx;  // Contadores
    int                  i=0;             //contador de mensajes
    pid_t pid_n;

    signal(SIGHUP,handler);
    server_s = socket(AF_INET, SOCK_STREAM, SOCKET_PROTOCOL);
    if (server_s==-1) {
        perror("socket");
        return 1;
    }
    printf("Creado el descriptor del socket %d\n",server_s);
  
    server_addr.sin_family      = AF_INET;            // Familia TCP/IP
    server_addr.sin_port        = htons(PORT_NUM);    // Número de Port, htons() lo convierte al orden de la red
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY = cualquier direccion IP, htonl() lo convierte al orden de la red
  
    bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Asociado el descriptor %u con el port %u\n", server_s,PORT_NUM);
  
    listen(server_s, NCONCUR);

    addr_len = sizeof(client_addr);
    while(!terminar) {   
        connect_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len);
        if (connect_s==-1) {
            perror("accept");
            return 2;
        }
        printf("Nueva conexión desde: %s:%hu\n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        if((pid_n=fork())==0) {
            do {
                printf("Proceso hijo %d atendiendo conexión desde: %s\n",getpid(),inet_ntoa(client_addr.sin_addr));
                close (server_s); // Cerrar socket no usado

                sprintf(buf_tx,"Listo"); // Mensaje de listo para recibir
                bytesaenviar =  strlen(buf_tx);
                bytestx=send(connect_s, buf_tx, bytesaenviar, 0);
                if (bytestx==-1) {
                    perror ("send");
                    return 3;
                }

                bytesrecibidos=recv(connect_s, buf_rx, sizeof(buf_rx), 0);
                if (bytesrecibidos==-1) {
                    perror ("recv");
                    return 3;
                }
                // Verificar que se recibe "archivo" primero, armar string con el nombre, y ver como manejar la recepcion del archivo
            } while (strncmp(buf_rx,"FIN",3)!=0); 
            close(connect_s);
            return 0; 
        } else {
            close(connect_s);
        } 
    }
    wait(NULL);
    close(server_s);
    return 0;
}


void handler(int sig) {
	if (sig==SIGHUP) {
	    terminar=1;
	    printf("señal HUP recibida, cuando establezca la proxima conexión el servidor terminará\n");
	}
}
