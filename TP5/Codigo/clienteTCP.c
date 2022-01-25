
#include <stdio.h>
#include <string.h>

#include <sys/types.h>    
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
/****************************************************************************
PROGRAMA CLIENTE TCP
****************************************************************************/
//----- Defines -------------------------------------------------------------
#define  PORT_NUM           1050  // Port number used
#define  IP_ADDR "127.0.0.1" // Direccion IP LOCALHOST
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

/*  leo argumentos desde la line de comandos*/
  if (argc!=2)
    {
    printf("uso: clienteTCP www.xxx.yyy.zzz\n");
    return -1; 
    }
    strncpy(ipserver,argv[1],16);
  
  // >>> Paso  #1 <<<
  
  
  // Crear el socket
  // AF_INET es  "Address Family: Internet (Familia de Direcciones TCP/IP) 
  // SOCK_DGRAM es para datagramas ( Protocolo UDP) 
  // SOCKET_PROTOCOL fue previamente #defined
  
  client_s = socket(AF_INET, SOCK_STREAM, SOCKET_PROTOCOL);
  if (client_s==-1)
    {
    perror("socket");
    return 2;
    }
  printf("Cree el descriptor del socket %d\n",client_s);
  /* >>> Paso #2 <<<
  relleno la estructura sock_addr con la información necesaria
  la estructura sockaddr_in está definida como sigue:

  struct sockaddr_in {
    short            sin_family;    Para sockets TCP/IP usamos  AF_INET
    unsigned short   sin_port;      Acá va el port, pero debemos ponerlo en el orden de bits de la red 
    struct in_addr   sin_addr;      Esta a su vez es otra estructura. ver más abajo
    char             sin_zero[8];   Esto está definido pero no se usa
    };

  struct in_addr {
      unsigned long s_addr;         Acá hay que poner la dirección IP como un unsigned de 32 bits.
                                    podemos usar las funciones inet_aton() o inet_addr().
      };
      int inet_aton(char *string_IP, struct in_addr *binario_IP)
        Toma como entrada una cadena string_IP con el número IP en quad-dot y devuelve el número IP en binario y en el orden 
        de la red en el campo correspondiente de la estructura  binario_IP
        devuelve 1 si la conversión tuvo éxito o devuelve 0 si la cadena era inválida.
      
      int inet_addr(char *string_IP)
        Toma como entrada una cadena string_IP con el número IP en quad-dot y devuelve el número IP en binario y en el 
        orden de la red. Su uso parece más simple que inet_aton() pero tiene el problema que devuelve -1 en caso que 
        la cadena sea inválida. Esto se confunde con el valor binario que corresponde a la dirección IP "255.255.255.255"
      
      Se recomienda usar inet_aton()
  */
  
  server_addr.sin_family      = AF_INET;            // Familia TCP/IP
  server_addr.sin_port        = htons(PORT_NUM);    // Número de Port, htons() lo convierte al orden de la red
  
  if  (inet_aton(ipserver, &server_addr.sin_addr)==0) // cargo la estructura con la dirección IP del servidor
    {
    printf ("La dirección  IP_ADDR no es una dirección IP válida. Programa terminado\n"); 
    return 3;
    }
  addr_len = sizeof(server_addr);
  /* >>> Paso  #3 <<<
  Conecto con el servidor si el intento da error (-1) salgo del programa, si no, comienzo el intercambio de datos
  */
   
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
    /* >>> Paso  #4 <<<
    fgets() lee caracteres desde stdin y los almacena en buf_tx. Número máximo de bytes es 1499 , dejo uno para el NULL
     que termina el string.
     luego voy a enviar SOLAMENTE los caracteres ingresados SIN el NULL de terminación, para eso cuento los caracteres 
     con strlen() y guardo ese número en bytesaenviar
     */
    
    if(cnt==0){
      bytesrecibidos=recv(client_s, buf_rx, sizeof(buf_rx), 0);
      buf_rx[bytesrecibidos]=0;  //Me aseguro que termine en NULL 
      if(!strncmp(buf_rx,"Listo",5)){
        printf("Se recibio la palabra %s\n", buf_rx);
        sprintf(buf_tx,"Archivo"); // Mensaje de listo para recibir
        bytesaenviar =  strlen(buf_tx);
        bytestx=send(client_s, buf_tx, bytesaenviar, 0);
      }
      else{
        printf("No se recibio la palabra Listo\n");
      }
    }
    

    printf("Ingrese mensaje\n");

    fgets(buf_tx,1499,stdin);
    bytesaenviar=strlen(buf_tx)+1; //me aseguro que la cantidad de bytes a enviar sea la correcta 
    buf_tx[bytesaenviar]=0;
    /* >>> Paso  #5 <<<
    imprimo el mensaje enviado en pantalla.
    */
    printf("******************************\n");
    printf("      Mensaje Enviado\n");
    printf("******************************\n");
    printf("%s\n",buf_tx);
    /* >>> Paso  #6 <<<
    Quedo a la espera de recibir el mensaje del servidor
    */
    /*bytesrecibidos=recv(client_s, buf_rx, sizeof(buf_rx), 0);
    buf_rx[bytesrecibidos]=0;  //Me aseguro que termine en NULL 
    
    printf("%s", buf_rx);
*/
    //* >>> Paso  #7 <<<
    // Envio al servidor,  
  
    bytestx=send(client_s, buf_tx, bytesaenviar,0);  
    cnt++;
    } while (strncmp(buf_tx,"FIN",3)!=0); 
    cnt=0;
/* >>> Paso 11 <<< 
En el while pongo como condición de salida que el mensaje enviado no comience con "FIN"
*/

/*
>>> Paso 12 <<<
Cierro el socket
*/
close(client_s);
return 0;
} // fin del programa
