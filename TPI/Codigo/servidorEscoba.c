#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h> 
#include <sys/wait.h>

#include <sys/stat.h>
#include <sys/types.h>    
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

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
time_t t;
struct tm tiempo_init;
struct tm tiempo_fin;

int contbytestx, contbytesrx, cerrar_archivo;
long int size;
FILE *fp;
unsigned int connect_s;

void handler(int sig);
void watchdog(int sig);
void writeLog(struct tm tiempo_i, struct tm tiempo_f, int status, int tam, int bytes_tx, int bytes_rx);

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
    char                 archivo[100];    // Variable con el nombre de archivo que se va a leer
    t = time(NULL);
    signal(SIGHUP,handler);
    signal(SIGALRM,watchdog);
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
        tiempo_init = *localtime(&t);
        printf("Nueva conexión desde: %s:%hu\n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        if(fork()==0) {
            long int auxsize;
            contbytesrx = 0;
            contbytestx = 0;
            printf("Proceso hijo %d atendiendo conexión desde: %s\n",getpid(),inet_ntoa(client_addr.sin_addr));
            close (server_s); // Cerrar socket no usado

            sprintf(buf_tx,"Listo"); // Mensaje de listo para recibir
            bytesaenviar =  strlen(buf_tx);
            bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
            if (bytestx==-1) {
                perror ("send");
                return 3;
            }
            contbytestx += bytestx;

            alarm(10);
            bytesrecibidos=recv(connect_s, buf_rx, sizeof(buf_rx), 0); // Chequear recepción de "archivo"
            alarm(0);
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
                contbytesrx += bytesrecibidos;
                char aux[100];
                char delimitador[] = " ";
                char *token = strtok(buf_rx, delimitador);
                strcpy(archivo,token);
                token = strtok(NULL, delimitador);
                strcpy(aux,token);
                size=atoi(aux);
                auxsize=size;
                printf("El nombre del archivo del cliente es: '%s' y tiene un tamaño de %ld bytes.\n", archivo, size);
                fp=fopen(archivo,"wb");
                cerrar_archivo=1;
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
                alarm(10);
                bytesrecibidos=recv(connect_s, buf_rx, sizeof(buf_rx), 0);
                alarm(0);
                if (bytesrecibidos==-1) {
                   perror ("recv");
                   return 3;
                }
                fwrite(buf_rx,sizeof(char),bytesrecibidos,fp);
                auxsize-=bytesrecibidos;
                contbytesrx += bytesrecibidos;
                printf("Recibi %d bytes del cliente.\n",bytesrecibidos);
                if(auxsize<0){
                    printf("Llegaron bytes de mas, terminando conexión.\n");
                    sprintf(buf_tx,"ERROR: Sobre-envío de datos.\n");
                    bytesaenviar = strlen(buf_tx);
                    bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
                    contbytestx += bytestx;
                    tiempo_fin = *localtime(&t);
                    writeLog(tiempo_init, tiempo_fin, 4, 0, contbytestx, contbytesrx);
                    return 0;
                }
            } while (auxsize!=0); 
            fclose(fp);
            struct stat st;
            if (stat(archivo, &st) == 0) {
                auxsize = st.st_size;
            }
            printf("Recepción terminada - Sin errores\n");
            printf("Archivo %s completo, tamaño declarado %ld bytes, tamaño real %ld bytes.\n", archivo, size, auxsize);
            sprintf(buf_tx,"Recepción terminada - Sin errores.\n");
            bytesaenviar = strlen(buf_tx);
            bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
            close(connect_s);
            contbytestx += bytestx;
            tiempo_fin = *localtime(&t);
            writeLog(tiempo_init, tiempo_fin, 0, size, contbytestx, contbytesrx);
            return 0;
        } else {
            close(connect_s);
        } 
    }
    wait(NULL);
    close(server_s);
    return 0;
}

void writeLog(struct tm tiempo_i,struct tm tiempo_f, int status, int tam, int bytes_tx, int bytes_rx) {
    FILE *log;
    log = fopen("connect_log.csv","a");
    if (log == 0) {
        printf("Error accediendo al archivo de log.\n");
        return;
    }
    switch (status) {
    case 0:
        fprintf(log,"%02d/%02d/%d, Inicio: %02d:%02d:%02d, Fin: %02d:%02d:%02d, Estado: Sin errores, Tamanio: %d, Enviado: %d, Recibido: %d\n",tiempo_i.tm_mday,tiempo_i.tm_mon+1,tiempo_i.tm_year+1900,tiempo_i.tm_hour,tiempo_i.tm_min,tiempo_i.tm_sec,tiempo_f.tm_hour,tiempo_f.tm_min,tiempo_f.tm_sec,tam,bytes_tx,bytes_rx);
        break;
    case 1:
        fprintf(log,"%02d/%02d/%d, Inicio: %02d:%02d:%02d, Fin: %02d:%02d:%02d, Estado: Instruccion incorrecta, Tamanio: %d, Enviado: %d, Recibido: %d\n",tiempo_i.tm_mday,tiempo_i.tm_mon+1,tiempo_i.tm_year+1900,tiempo_i.tm_hour,tiempo_i.tm_min,tiempo_i.tm_sec,tiempo_f.tm_hour,tiempo_f.tm_min,tiempo_f.tm_sec,tam,bytes_tx,bytes_rx);
        break;
    case 2:
        fprintf(log,"%02d/%02d/%d, Inicio: %02d:%02d:%02d, Fin: %02d:%02d:%02d, Estado: Timeout, Tamanio: %d, Enviado: %d, Recibido: %d\n",tiempo_i.tm_mday,tiempo_i.tm_mon+1,tiempo_i.tm_year+1900,tiempo_i.tm_hour,tiempo_i.tm_min,tiempo_i.tm_sec,tiempo_f.tm_hour,tiempo_f.tm_min,tiempo_f.tm_sec,tam,bytes_tx,bytes_rx);
        break;
    case 3:
        fprintf(log,"%02d/%02d/%d, Inicio: %02d:%02d:%02d, Fin: %02d:%02d:%02d, Estado: Sub-recepcion, Tamanio: %d, Enviado: %d, Recibido: %d\n",tiempo_i.tm_mday,tiempo_i.tm_mon+1,tiempo_i.tm_year+1900,tiempo_i.tm_hour,tiempo_i.tm_min,tiempo_i.tm_sec,tiempo_f.tm_hour,tiempo_f.tm_min,tiempo_f.tm_sec,tam,bytes_tx,bytes_rx);
        break;
    case 4:
        fprintf(log,"%02d/%02d/%d, Inicio: %02d:%02d:%02d, Fin: %02d:%02d:%02d, Estado: Sobre-recepcion, Tamanio: %d, Enviado: %d, Recibido: %d\n",tiempo_i.tm_mday,tiempo_i.tm_mon+1,tiempo_i.tm_year+1900,tiempo_i.tm_hour,tiempo_i.tm_min,tiempo_i.tm_sec,tiempo_f.tm_hour,tiempo_f.tm_min,tiempo_f.tm_sec,tam,bytes_tx,bytes_rx);
        break;
    default:
        break;
    }
    fclose(log);
    return;
}

void handler(int sig) {
	if (sig==SIGHUP) {
	    terminar=1;
	    printf("Señal HUP recibida, la proxima conexión es la última.\n");
	}
}

void watchdog(int sig) {
    int bytesaenviar,bytestx;
    char buf_tx[1500];
    if (bytestx==-1) {
        perror ("send");
        terminar = 1;
    }
    tiempo_fin = *localtime(&t);
    if (cerrar_archivo == 1) {
        fclose(fp);
        printf("Error en la conexión\n");
        sprintf(buf_tx,"ERROR: Se enviaron bytes de menos.\n");
        bytesaenviar = strlen(buf_tx);
        bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
        contbytestx += bytestx;
        writeLog(tiempo_init, tiempo_fin, 3, size, contbytestx, contbytesrx);
    } else {
        printf("Error en la conexión\n");
        sprintf(buf_tx,"ERROR: Timeout en la conexión.\n");
        bytesaenviar = strlen(buf_tx);
        bytestx = send(connect_s, buf_tx, bytesaenviar, 0);
        contbytestx += bytestx;
        writeLog(tiempo_init, tiempo_fin, 2, 0, contbytestx, contbytesrx);
    }
    close(connect_s);
    terminar = 1;
    return;
}
