#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/socket.h>  
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

/* Basado (based) en la consulta:
El mazo, las manos y las cartas son memoria compartida, pero no es necesario que se sincronice, los hijos solo leen, el padre escribe
    Hijos escriben: Jugadores , leen: mano, cartas_mesa,
    Padre escribe: mano, cartas_mesa , lee: Jugadores
Se pueden usar colas de msj por ej para mandarle al padre que jugada hace cada cliente, asi los hijos no tienen que escribir mem comp.
Los hijos tienen que ser lo mas modular posible, preguntar nombre, y esperar el turno, hay que ver como sincronizar lo del turno, tal vez con signals
El padre tiene que preparar la memoria compartida y ponerse a esperar las conexiones para derivar a los hijos, una vez que estan todos conectados
se puede repartir las cartas y demas.
*/

//******************************* Defines *******************************//
#define  PORT_NUM 1234  // Numero de Port
#define  IP_ADDR "127.0.0.1" // Direccion IP LOCALHOST
#define NCONCUR 4
#define SOCKET_PROTOCOL 0

#define NUMERO 30      
#define PATH "/dev/null"

#define SEND_RECV() {\
    bytesaenviar = strlen(buf_tx);\
    bytestx = send(connect_s, buf_tx, bytesaenviar, 0);\
    bytesrecibidos=recv(connect_s, buf_rx, sizeof(buf_rx), 0);\
}

#define SEND() {\
    bytesaenviar = strlen(buf_tx);\
    bytestx = send(connect_s, buf_tx, bytesaenviar, 0);\
}

//******************************** Global *******************************//
int terminar = 0;
unsigned int connect_s;

//******************************* Main program *******************************//
int main(int argc, char *argv[]) {

    unsigned int         server_s;        // Descriptor del socket
    struct sockaddr_in   server_addr;     // Estructura con los datos del servidor
    struct sockaddr_in   client_addr;     // Estructura con los datos del cliente
    struct in_addr       client_ip_addr;  // Client IP address
    int                  addr_len;        // Tamaño de las estructuras
    char                 buf_tx[1500],buf_rx[1500];
    int                  bytesrecibidos, bytesaenviar, bytestx;  // Contadores
    int                  cant_jug=1;
    char                 jugadores[4][50];
    int                  mazo[40];         //0 - 9 basto, 10 - 19 espada, 20 - 29 copa, 30 - 39 oro
    int                  cartas_mesa[10];
    int                  mano[4][3];       // Despues tenemos que cambiarlo para que varie con la cantidad de jugadores
    int                  cartas_levantadas[4][20];   // Variable que contiene las cartas que cada jugador levanto
    int                  jugador=0;        // Variable para indicar el turno
    int                  suma_mano=0;
    int                  i,j,k;
    key_t                clave1, clave2;
    int                  IDmem1, IDmem2;

    int *mano = NULL; //Va a ser un array de 12, para recorrerlo usar mano[jugador*3+i], con i de 0 a 2, jug 1 es de 0 a 2, jug 2 de 3 a 5, y asi hasta 9 a 11 para el 4to -JP
    int *cartas_mesa = NULL;

    /*************************************** Obtención de la memoria compartida ***************************************/
    //(No agregué colas de msj todavia por si no las necesitamos -JP)
    clave1 = ftok(PATH,NUMERO);
    if (clave1 == (key_t) -1){
		printf("No se pudo obtener una clave\n");
		exit(1);
	}
    clave2 = ftok(PATH,NUMERO+1);
    if (clave2 == (key_t) -1){
		printf("No se pudo obtener una clave\n");
		exit(1);
	}

    // Llamar al sistema para obtener la memoria compartida
    IDmem1 = shmget(clave1, 12*sizeof(int), 0666 | IPC_CREAT);
    if(IDmem1 == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}
    IDmem2 = shmget(clave2, 10*sizeof(int), 0666 | IPC_CREAT); // Hasta 10 cartas en la mesa a la vez? Parece razonable, se puede cambiar -JP
    if(IDmem2 == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    // Adosar el proceso a los espacios de memoria mediante un puntero
    mano = (int *) shmat(IDmem1, (const void *)0,0);
    if (mano == NULL){
		printf("No se pudo asociar el puntero a la memoria compartida\n");
		exit(3);
	}
    cartas_mesa = (int *) shmat(IDmem2, (const void *)0,0);
    if (cartas_mesa == NULL){
		printf("No se pudo asociar el puntero a la memoria compartida\n");
		exit(3);
	}

    /************************************** Creación del socket para el servidor **************************************/

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

    /******************************* Creacion de la partida y conexion de los jugadores *******************************/

    addr_len = sizeof(client_addr);
    while(!terminar) { // Loop general, cuando termina una partida vuelve acá para iniciar otra
        while(jugador<cant_jug) {      // Loop de conexión de jugadores
            connect_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len);
            if (connect_s==-1) {
                perror("accept");
                return 2;
            }
            printf("Nueva conexión desde: %s:%hu , Jugador %d.\n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),i+1);
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
                close(connect_s);
            }
            if (fork()==0) { // Cada hijo se queda con su cliente, cuando es el turno tiene que entrar en el "loop de la jugada" (Levantar, descartar, que cartas, etc)
                sprintf(buf_tx,"Jugador %d de %d. Ingrese su nombre: ",jugador+1,cant_jug);
                SEND_RECV();
                for (j=0;j<strlen(buf_rx);j++) if (buf_rx[j] == '\n') buf_rx[j] == '\0';
                strcpy(jugadores[jugador],buf_rx);
            } else {
                jugador++;
                close(connect_s);
            }
        }
        // Loop de la partida
    } /* A partir de acá hay que revisar */

    int numero_aleatorio = (int)(rand() % 40);      // Esto devuelve un número entre 0 y 39       
    int cartas_jugadas = 0;
    int primera_mano = 1;
    for(k=0; k<3; k++){                         //Asignamos que los jugadores no tengan cartas
        for (j=0; j<cant_jug; j++){ 
            mano[j][k] = 40;

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
        int numero_aleatorio = (int)(rand() % 40);      // Esto devuelve un número entre 0 y 39       
        int cartas_jugadas = 0;
        int primera_mano = 1;
        for(int k=0; k<3; k++){                         //Asignamos que los jugadores no tengan cartas
            for (int j=0; j<cant_jug; j++){ 
                mano[j][k] = 40;
            }
        }
        
    while(cartas_jugadas!=40){            
        // Reparto de cartas a cada jugador (3 por jugador)
        for(k=0; k<3; k++){
            for (j=0; j<cant_jug; j++){ 
                while(mazo[numero_aleatorio]==1){
                    numero_aleatorio = (int)(rand() % 40);
        while(cartas_jugadas!=40){            
            // Reparto de cartas a cada jugador (3 por jugador)
            for(int k=0; k<3; k++){
                for (int j=0; j<cant_jug; j++){ 
                    while(mazo[numero_aleatorio]==1){
                        numero_aleatorio = (int)(rand() % 40);
                    }
                    mazo[numero_aleatorio]=1;
                    mano[j][k] = numero_aleatorio;
                }
            }
        }
        // Si es la primera mano repartir en la mesa
        if (primera_mano == 1) {
            primera_mano = 0;
            for (j=0; j<4; j++){ 
                while(mazo[numero_aleatorio]==1){
                    numero_aleatorio = (int)(rand() % 40);

            // Si es la primera mano repartir en la mesa
            if (primera_mano == 1) {
                primera_mano = 0;
                for (int j=0; j<4; j++){ 
                    while(mazo[numero_aleatorio]==1){
                        numero_aleatorio = (int)(rand() % 40);
                    }
                    mazo[numero_aleatorio]=1;
                    cartas_mesa[j]=numero_aleatorio;
                }
            }

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
            // Si es la primera mano repartir en la mesa
            
            while(jugador<cant_jug && suma_mano!=120){     // Cuando el jugador descarta una carta, el numero de la carta se reemplaza por 40 en la variable mano

        while(jugador<cant_jug && suma_mano!=120){     // Cuando el jugador descarta una carta, el numero de la carta se reemplaza por 40 en la variable mano

            for(int j=0; j<40; j++){                    // Contar cuantas cartas hay repartidas         
                cartas_jugadas += mazo[j];
            }
            
            while(jugador<cant_jug && suma_mano!=120){     // Cuando el jugador descarta una carta, el numero de la carta se reemplaza por 40 en la variable mano

                // Codigo del juego

                // Avisarle al jugador correspondiente las cartas que tiene y las que estan en la mesa
                
                jugador++;
                if (jugador-1 == cant_jug){
                    suma_mano = mano[jugador-1][0] + mano[jugador-1][1] + mano[jugador-1][2];
                    jugador = 0;
                }
            }

            cartas_jugadas = 0;
            for(int j=0; j<40; j++){
                cartas_jugadas += cartas_mesa[j];
            }

            cartas_jugadas = 0;
            /* for(int j=0; j<40; j++){
                cartas_jugadas += cartas_mesa[j];
            } */
        }
        /* De acá en adelante sigue siendo código viejo */


        /* for(int j=0; j<40; j++){
            cartas_jugadas += cartas_mesa[j];
        } */
    }
    /* Mostrar puntajes */

        if(fork()==0) {
            do {
    
            } while (1); 
            
            return 0;
        } else {
            
        } 
    wait(NULL);
    close(server_s);
    return 0;

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