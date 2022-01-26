
#include <stdio.h>
#include <string.h>

#include <sys/types.h>    
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
  int cnt=0;
  do{  
    
    if(cnt==0){
      bytesrecibidos=recv(client_s, buf_rx, sizeof(buf_rx), 0);
      buf_rx[bytesrecibidos]=0;  //Me aseguro que termine en NULL 
      if(!strncmp(buf_rx,"Listo",5)){
        printf("Se recibio la palabra %s\n", buf_rx);
        // Envio la palabra "archivo"
        sprintf(buf_tx,"Archivo");
        bytesaenviar =  strlen(buf_tx);
        bytestx=send(client_s, buf_tx, bytesaenviar, 0);
      }
      else{
        printf("No se recibio la palabra Listo\n");
        return 5;
      }
    }

    printf("Ingrese nombre del archivo\n");
    fgets(archivo,100,stdin);

    fp = fopen(archivo,"rb");
    if(fp == NULL){
      perror("openfile");
      printf("No se pudo abrir el archivo.\n")
      return 6;
    }

    while (fp != EOF){
      fgets(buf_tx,1500,fp);
      bytesaenviar=strlen(buf_tx);
      bytestx=send(client_s,buf_tx,bytesaenviar,0);
      // Lectura del archivo para enviar al servidor
    }

    fclose(fp);

    fgets(buf_tx,1499,stdin);
    bytesaenviar=strlen(buf_tx)+1; //me aseguro que la cantidad de bytes a enviar sea la correcta 
    buf_tx[bytesaenviar]=0;

    printf("******************************\n");
    printf("      Mensaje Enviado\n");
    printf("******************************\n");
    printf("%s\n",buf_tx);  
  
    bytestx=send(client_s, buf_tx, bytesaenviar,0);  
    cnt++;
  } while (strncmp(buf_tx,"FIN",3)!=0); 
  cnt=0;

  close(client_s);
  return 0;
}
