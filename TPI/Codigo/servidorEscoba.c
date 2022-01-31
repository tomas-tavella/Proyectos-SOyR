#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>    
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>

/* Basado en los requerimientos:
El proceso padre del server se queda con el player 1, los hijos atienden del 2 al 4
El padre prepara la memoria compartida y espera a que se conecten los demas
Los naipes se pueden hacer con un vector de 40 char, int o lo que sea, y si la carta esta repartida ese numero se pone en 1 sino en 0 por ej, se pueden repartir con un rand(40)
El padre maneja el orden de jugada y quien juega actualmente
*/


//----- Defines -------------------------------------------------------------
#define  PORT_NUM 1234  // Numero de Port
#define  IP_ADDR "127.0.0.1" // Direccion IP LOCALHOST
#define NCONCUR 4
#define SOCKET_PROTOCOL 0

/*-----------------Global---------------*/
int terminar = 0;
unsigned int connect_s;

//===== Main program ========================================================
int main(int argc, char *argv[]) {

    unsigned int         server_s;        // Descriptor del socket
    struct sockaddr_in   server_addr;     // Estructura con los datos del servidor
    struct sockaddr_in   client_addr;     // Estructura con los datos del cliente
    struct in_addr       client_ip_addr;  // Client IP address
    int                  addr_len;        // Tamaño de las estructuras
    char                 buf_tx[1500];    // Buffer de 1500 bytes para los datos a transmitir
    char                 buf_rx[1500];    // Buffer de 1500 bytes para los datos a transmitir
    int                  bytesrecibidos, bytesaenviar, bytestx;  // Contadores

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
    printf("Servidor en proceso %d listo.\n",getpid());
    listen(server_s, NCONCUR);

    addr_len = sizeof(client_addr);
    while(!terminar) {   
        connect_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len);
        if (connect_s==-1) {
            perror("accept");
            return 2;
        }
        printf("Nueva conexión desde: %s:%hu\n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        if(fork()==0) {
            printf("Proceso hijo %d atendiendo conexión desde: %s\n",getpid(),inet_ntoa(client_addr.sin_addr));
            close (server_s); // Cerrar socket no usado

            sprintf(buf_tx,"Listo"); // Mensaje de listo para recibir
            bytesaenviar =  strlen(buf_tx);
            bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
            if (bytestx==-1) {
                perror ("send");
                return 3;
            }

            bytesrecibidos=recv(connect_s, buf_rx, sizeof(buf_rx), 0); // Chequear recepción de "archivo"
            if (bytesrecibidos==-1) {
                perror ("recv");
                return 3;
            }
            contbytesrx += bytesrecibidos;
            if (!strncmp(buf_rx,"Archivo",7)) {
                printf("Se recibio la palabra %s\n", buf_rx);
                alarm(10);
                bytesrecibidos=recv(connect_s, buf_rx, sizeof(buf_rx), 0);
                alarm(0);
                if (bytesrecibidos==-1) {
                    perror ("recv");
                    return 3;
                }
            } else {
                printf("Error en la conexión.\n");
                sprintf(buf_tx,"ERROR: No se recibió la palabra 'Archivo'.\n");
                bytesaenviar = strlen(buf_tx);
                bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
                if (bytestx==-1) {
                    perror ("send");
                    return 3;
                }
                contbytestx += bytestx;
                tiempo_fin = *localtime(&t);
                writeLog(tiempo_init, tiempo_fin, 1, 0, contbytestx, contbytesrx);
                close(connect_s);
                return 0;
            }
            do {
                bytesrecibidos=recv(connect_s, buf_rx, sizeof(buf_rx), 0);
                if (bytesrecibidos==-1) {
                   perror ("recv");
                   return 3;
                }
                fwrite(buf_rx,sizeof(char),bytesrecibidos,fp);
                printf("Recibi %d bytes del cliente.\n",bytesrecibidos);
                if(auxsize<0){
                    printf("Llegaron bytes de mas, terminando conexión.\n");
                    sprintf(buf_tx,"ERROR: Sobre-envío de datos.\n");
                    bytesaenviar = strlen(buf_tx);
                    bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
                    return 0;
                }
            } while (auxsize!=0); 
            printf("Recepción terminada - Sin errores\n");
            printf("Archivo %s completo, tamaño declarado %ld bytes, tamaño real %ld bytes.\n", archivo, size, auxsize);
            sprintf(buf_tx,"Recepción terminada - Sin errores.\n");
            bytesaenviar = strlen(buf_tx);
            bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
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