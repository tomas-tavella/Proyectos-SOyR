#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h> 
#include <sys/wait.h> 
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


//******************************* Defines *******************************//
#define  PORT_NUM 1234  // Numero de Port
#define  IP_ADDR "127.0.0.1" // Direccion IP LOCALHOST
#define NCONCUR 4
#define SOCKET_PROTOCOL 0
#define SIETE_ORO 36

#define SEND_RECV() {\
    bytesaenviar = strlen(buf_tx);\
    bytestx = send(connect_s[i], buf_tx, bytesaenviar, 0);\
    bytesrecibidos=recv(connect_s[i], buf_rx, sizeof(buf_rx), 0);\
}

#define SEND() {\
    bytesaenviar = strlen(buf_tx);\
    bytestx = send(connect_s[i], buf_tx, bytesaenviar, 0);\
}

//******************************** Global *******************************//
int terminar = 0;
unsigned int connect_s[4];

//******************************* Main program *******************************//
int main(int argc, char *argv[]) {

    unsigned int         server_s;        // Descriptor del socket
    struct sockaddr_in   server_addr;     // Estructura con los datos del servidor
    struct sockaddr_in   client_addr;     // Estructura con los datos del cliente
    struct in_addr       client_ip_addr;  // Client IP address
    int                  addr_len;        // Tamaño de las estructuras
    char                 buf_tx[1500];    // Buffer de 1500 bytes para los datos a transmitir
    char                 buf_rx[1500];    // Buffer de 1500 bytes para los datos a transmitir
    int                  bytesrecibidos, bytesaenviar, bytestx;  // Contadores
    int                  cant_jug;
    char                 jugadores[4][50];
    int                  mazo[40];         //0 - 9 basto, 10 - 19 espada, 20 - 29 copa, 30 - 39 oro
    int                  cartas_mesa[10];
    int                  mano[4][3];       // Despues tenemos que cambiarlo para que varie con la cantidad de jugadores
    int                  cartas_levantadas[4][20];   // Variable que contiene las cartas que cada jugador levanto
    int                  jugador=0;        // Variable para indicar el turno
    int                  suma_mano=0;

    server_s = socket(AF_INET, SOCK_STREAM, SOCKET_PROTOCOL);
    if (server_s==-1) {
        perror("socket");
        return 1;
    }
    srand(time(0));
    printf("Servidor de escoba de 15.\n");
    printf("Creado el descriptor del socket %d\n",server_s);
  
    server_addr.sin_family      = AF_INET;            // Familia TCP/IP
    server_addr.sin_port        = htons(PORT_NUM);    // Número de Port, htons() lo convierte al orden de la red
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY = cualquier direccion IP, htonl() lo convierte al orden de la red
  
    bind(server_s, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Asociado el descriptor %u con el port %u\n", server_s,PORT_NUM);
    printf("Servidor en proceso %d listo.\n",getpid());
    listen(server_s, NCONCUR);
    volatile int i = 0, j = 0, k = 0;

    //******************************* Creacion de la partida y conexion de los jugadores *******************************//

    addr_len = sizeof(client_addr);
    while(i<cant_jug) {   
        connect_s[i] = accept(server_s, (struct sockaddr *)&client_addr, &addr_len);
        if (connect_s[i]==-1) {
            perror("accept");
            return 2;
        }
        printf("Nueva conexión desde: %s:%hu , Jugador %d.\n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),i+1);
        if (i == 0) {
            sprintf(buf_tx,"Bienvenido a la escoba de 15 MP, por favor ingrese el número de jugadores (2-4): ");
            SEND_RECV();
            if (bytesrecibidos==-1) {
                perror ("recv");
                return 3;
            }
            cant_jug = atoi(buf_rx);
            if (cant_jug > 4 || cant_jug < 2) {
                sprintf(buf_tx,"El número de jugadores solicitado es inválido.\n");
                SEND();
                close(connect_s[i]);
            }
            sprintf(buf_tx,"Ingrese su nombre: ");
            SEND_RECV();
            for (j=0;j<strlen(buf_rx);j++) if (buf_rx[j] == '\n') buf_rx[j] == '\0';
            strcpy(jugadores[i],buf_rx);
            printf("El jugador %s creo la partida para %d jugadores\n", jugadores[i], cant_jug);
<<<<<<< HEAD
        } else if(i<cant_jug ) {
            sprintf(buf_tx,"Bienvenido a la escoba de 15 MP, ingrese su nombre: ");
            SEND_RECV();
            strncpy(jugadores[i],buf_rx,bytesrecibidos);
            printf("El jugador %s es el numero %d\n", jugadores[i], i+1);
=======
            //i++;
        } else if(i<cant_jug ) { /* Hacer un fork y atender a los otros jugadores */
            if(fork()==0){
                sprintf(buf_tx,"Ingrese su nombre: ");
                SEND_RECV();
                strncpy(jugadores[i],buf_rx,bytesrecibidos);
                printf("El jugador %s es el numero %d\n", jugadores[i], i+1);
            }
            else{
                close(connect_s[i]);
            }
            wait(NULL);
>>>>>>> parent of dd76475... Agregado mega cheat de simplificacion (Un proceso atiende a todos los clientes, no son necesarios los hijos)
        }
        i++;
        if(i==cant_jug){
            //printf("La partida esta a punto de comenzar\nJugador 1: %s\nJugador 2: %sJugador 3: %sJugador 4: %s\nLa suerte es techada\n", jugadores[0])
            printf("La partida esta a punto de comenzar\n");
            for (j=0; j<cant_jug; j++){
                printf("Jugador %d: %s\n",j+1,jugadores[j]);
            }
            printf("La suerte es techada\n");
        }
    }
<<<<<<< HEAD
<<<<<<< HEAD
    //************************************* Desarrollo del juego *************************************//
    int numero_aleatorio = (int)(rand() % 40);      // Esto devuelve un número entre 0 y 39       
    int cartas_jugadas = 0;
    int primera_mano = 1;
    for(k=0; k<3; k++){                         //Asignamos que los jugadores no tengan cartas
        for (j=0; j<cant_jug; j++){ 
            mano[j][k] = 40;
=======
        //************************************* Desarrollo del juego *************************************//

        // Reparto de cartas al centro (4 cartas)
        int numero_aleatorio = (int)(rand() % 40);             
        for (int j=0; j<cant_jug; j++){ 
            while(mazo[numero_aleatorio]==1){
                numero_aleatorio = (int)(rand() % 40);
            }
            mazo[numero_aleatorio]=1;
            cartas_mesa[j]=numero_aleatorio;
        }
        int cartas_jugadas = 0;
        for(int j=0; j<40; j++){
            cartas_jugadas += mazo[j];
        }
        for(int k=0; k<3; k++){                         //Asignamos que los jugadores no tengan cartas
            for (int j=0; j<cant_jug; j++){ 
                mano[j][k] = 40;         // ?
            }
>>>>>>> parent of f250f9c (Agregada funcion para traducir de numero del array a nombre de la carta)
=======
        //************************************* Desarrollo del juego *************************************//
        int numero_aleatorio = (int)(rand() % 40);      // Esto devuelve un número entre 0 y 39       
        int cartas_jugadas = 0;
        int primera_mano = 1;
        for(int k=0; k<3; k++){                         //Asignamos que los jugadores no tengan cartas
            for (int j=0; j<cant_jug; j++){ 
                mano[j][k] = 40;
            }
>>>>>>> parent of dd76475... Agregado mega cheat de simplificacion (Un proceso atiende a todos los clientes, no son necesarios los hijos)
        }
        
<<<<<<< HEAD
    while(cartas_jugadas!=40){            
        // Reparto de cartas a cada jugador (3 por jugador)
        for(k=0; k<3; k++){
            for (j=0; j<cant_jug; j++){ 
                while(mazo[numero_aleatorio]==1){
                    numero_aleatorio = (int)(rand() % 40);
=======
        while(cartas_jugadas!=40){            
            // Reparto de cartas a cada jugador (3 por jugador)
            for(int k=0; k<3; k++){
                for (int j=0; j<cant_jug; j++){ 
                    while(mazo[numero_aleatorio]==1){
                        numero_aleatorio = (int)(rand() % 40);
                    }
                    mazo[numero_aleatorio]=1;
                    mano[j][k] = numero_aleatorio;
>>>>>>> parent of dd76475... Agregado mega cheat de simplificacion (Un proceso atiende a todos los clientes, no son necesarios los hijos)
                }
            }
<<<<<<< HEAD
<<<<<<< HEAD
        }
        // Si es la primera mano repartir en la mesa
        if (primera_mano == 1) {
            primera_mano = 0;
            for (j=0; j<4; j++){ 
                while(mazo[numero_aleatorio]==1){
                    numero_aleatorio = (int)(rand() % 40);
=======
            // Si es la primera mano repartir en la mesa
            if (primera_mano == 1) {
                primera_mano = 0;
                for (int j=0; j<4; j++){ 
                    while(mazo[numero_aleatorio]==1){
                        numero_aleatorio = (int)(rand() % 40);
                    }
                    mazo[numero_aleatorio]=1;
                    cartas_mesa[j]=numero_aleatorio;
>>>>>>> parent of dd76475... Agregado mega cheat de simplificacion (Un proceso atiende a todos los clientes, no son necesarios los hijos)
                }
            }
<<<<<<< HEAD
        }
        for(j=0; j<40; j++){                    // Contar cuantas cartas hay repartidas         
            cartas_jugadas += mazo[j];
        }

        for(i=0;i<cant_jug;i++) {
            sprintf(buf_tx,"Tus cartas son: ");
            for (j=0;j<3;j++) {
                traducirCarta(buf_tx,mano[i][j]);
                strcat(buf_tx,", ");
            }
            strcat(buf_tx,"\n");
            SEND();
        }

        sprintf(buf_tx,"Las cartas sobre la mesa son: ");
        for (j=0;j<4;j++) {
            traducirCarta(buf_tx,cartas_mesa[j]);
            strcat(buf_tx,", ");
        }
        strcat(buf_tx,"\n");
        for(i=0;i<cant_jug;i++) {
            SEND();
        }
        
        while(1);
=======
            // Si es la primera mano repartir en la mesa
            
            while(jugador<cant_jug && suma_mano!=120){     // Cuando el jugador descarta una carta, el numero de la carta se reemplaza por 40 en la variable mano
>>>>>>> parent of f250f9c (Agregada funcion para traducir de numero del array a nombre de la carta)

        while(jugador<cant_jug && suma_mano!=120){     // Cuando el jugador descarta una carta, el numero de la carta se reemplaza por 40 en la variable mano
=======
            for(int j=0; j<40; j++){                    // Contar cuantas cartas hay repartidas         
                cartas_jugadas += mazo[j];
            }
            
            while(jugador<cant_jug && suma_mano!=120){     // Cuando el jugador descarta una carta, el numero de la carta se reemplaza por 40 en la variable mano
>>>>>>> parent of dd76475... Agregado mega cheat de simplificacion (Un proceso atiende a todos los clientes, no son necesarios los hijos)

                // Codigo del juego

                // Avisarle al jugador correspondiente las cartas que tiene y las que estan en la mesa
                
                jugador++;
                if (jugador-1 == cant_jug){
                    suma_mano = mano[jugador-1][0] + mano[jugador-1][1] + mano[jugador-1][2];
                    jugador = 0;
                }
            }
<<<<<<< HEAD
<<<<<<< HEAD
=======

            cartas_jugadas = 0;
            for(int j=0; j<40; j++){
                cartas_jugadas += cartas_mesa[j];
            }
>>>>>>> parent of f250f9c (Agregada funcion para traducir de numero del array a nombre de la carta)
=======

            cartas_jugadas = 0;
            /* for(int j=0; j<40; j++){
                cartas_jugadas += cartas_mesa[j];
            } */
>>>>>>> parent of dd76475... Agregado mega cheat de simplificacion (Un proceso atiende a todos los clientes, no son necesarios los hijos)
        }
        /* De acá en adelante sigue siendo código viejo */

<<<<<<< HEAD
        /* for(int j=0; j<40; j++){
            cartas_jugadas += cartas_mesa[j];
        } */
    }
    /* Mostrar puntajes */
=======
        if(fork()==0) {
            do {
    
            } while (1); 
            
            return 0;
        } else {
            
        } 
>>>>>>> parent of dd76475... Agregado mega cheat de simplificacion (Un proceso atiende a todos los clientes, no son necesarios los hijos)
    wait(NULL);
    close(server_s);
    return 0;
<<<<<<< HEAD
}

void traducirCarta (char * carta, int num) {
    //Función para traducir el número en el nombre completo de la carta
    int aux = num % 10;
    switch (aux) {
        case 0:
            strcat(carta,"Uno de ");
            break;
        case 1:
            strcat(carta,"Dos de ");
            break;
        case 2:
            strcat(carta,"Tres de ");
            break;
        case 3:
            strcat(carta,"Cuatro de ");
            break;
        case 4:
            strcat(carta,"Cinco de ");
            break;
        case 5:
            strcat(carta,"Seis de ");
            break;
        case 6:
            strcat(carta,"Siete de ");
            break;
        case 7:
            strcat(carta,"Sota de ");
            break;
        case 8:
            strcat(carta,"Caballo de ");
            break;
        case 9:
            strcat(carta,"Rey de ");
            break;
        default:
            break;
    }
    aux = num / 10;
    switch (aux) {
    case 0:
        strcat(carta,"bastos");
        break;
    case 1:
        strcat(carta,"espadas");
        break;
    case 2:
        strcat(carta,"copas");
        break;
    case 3:
        strcat(carta,"oros");
        break;
    default:
        break;
    }
    return;
=======
>>>>>>> parent of f250f9c (Agregada funcion para traducir de numero del array a nombre de la carta)
}