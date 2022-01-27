#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

//----- Defines -------------------------------------------------------------
#define  PORT_NUM 1050  // Port number used
#define SOCKET_PROTOCOL 0
//===== Main program ========================================================
int main(int argc, char *argv[])
{

  unsigned int         client_s;        // Descriptor del socket
  struct sockaddr_in   server_addr;     // Estructura con los datos del servidor
  struct sockaddr_in   client_addr;     // Estructura con los datos del cliente
  int                  addr_len;        // Tamaño de las estructuras
  char                 buf_tx[1500];    // Buffer de 1500 bytes para los datos a transmitir
  char                 buf_rx[1500];    // Buffer de 1500 bytes para los datos a transmitir
  char                 ipserver[16];
  int                  bytesrecibidos,bytesaenviar, bytestx;               // Contadores
  int                  conectado; //variable auxiliar
  char                 archivo[100];    // Variable con el nombre de archivo que se va a leer
  FILE                 *fp;
  off_t                size;

  if (argc!=2)
    {
    printf("uso: clienteTCP www.xxx.yyy.zzz\n");
    return -1; 
    }
    strncpy(ipserver,argv[1],16);
  
  client_s = socket(AF_INET, SOCK_STREAM, SOCKET_PROTOCOL);
  if (client_s==-1)
    {
    perror("socket");
    return 2;
    }
  printf("Cree el descriptor del socket %d\n",client_s);
  
  server_addr.sin_family = AF_INET;            // Familia TCP/IP
  server_addr.sin_port   = htons(PORT_NUM);    // Número de Port, htons() lo convierte al orden de la red
  
  if  (inet_aton(ipserver, &server_addr.sin_addr)==0) // cargo la estructura con la dirección IP del servidor
    {
    printf ("La dirección  IP_ADDR no es una dirección IP válida. Programa terminado\n"); 
    return 3;
    }
  addr_len = sizeof(server_addr);
   
  conectado=connect(client_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (conectado==-1)
    {
    perror("connect");
    return 4;
    }
  printf("El IP del servidor es: %s y el port del servidor  es %hu \n",inet_ntoa(server_addr.sin_addr),
  ntohs(server_addr.sin_port));

  bytesrecibidos=recv(client_s, buf_rx, sizeof(buf_rx), 0);
  buf_rx[bytesrecibidos]=0;  //Me aseguro que termine en NULL 
  if(!strncmp(buf_rx,"Listo",5)){
    printf("Se recibio la palabra %s\n", buf_rx);
    // Envio la palabra "archivo"
    sprintf(buf_tx,"Archivo");
    bytesaenviar =  strlen(buf_tx);
    bytestx=send(client_s, buf_tx, bytesaenviar, 0);
  }else{
    printf("No se recibio la palabra Listo\n");
    sprintf(buf_tx,"FIN");
    bytesaenviar =  strlen(buf_tx);
    bytestx=send(client_s, buf_tx, bytesaenviar, 0);
    return 5;
  }

  printf("Ingrese nombre del archivo\n");
  fgets(archivo,100,stdin);
  for (int i=0;i<100;i++) {
    if (archivo[i]=='\n') archivo[i]=0;
  }
  fp = fopen(archivo,"rb");
  if(fp == NULL){
    perror("openfile");
    printf("No se pudo abrir el archivo.\n");
    printf("%s",archivo);
    sprintf(buf_tx,"FIN");
    bytesaenviar =  strlen(buf_tx);
    bytestx=send(client_s, buf_tx, bytesaenviar, 0);
    return 6;
  } else {  //Enviar nombre del archivo, obtener tamaño y enviarlo
    sprintf(buf_tx,"%s",archivo);
    bytesaenviar =  strlen(buf_tx);
    bytestx=send(client_s, buf_tx, bytesaenviar, 0);
    struct stat st;
    if (stat(archivo, &st) == 0) {
        size = st.st_size;
        sprintf(buf_tx,"%ld",size);
        bytesaenviar = strlen(buf_tx);
        buf_tx[bytesaenviar] = 0;
        bytestx=send(client_s, buf_tx, bytesaenviar, 0);
    }
  }
  printf("Presione enter para enviar los datos.");
  fgets(archivo,100,stdin);
  while (!feof(fp)){
    // Lectura del archivo para enviar al servidor
    fgets(buf_tx,1500,fp);
    bytesaenviar=strlen(buf_tx);
    bytestx=send(client_s,buf_tx,bytesaenviar,0);
  }

  fclose(fp);
  close(client_s);
  return 0;
}
