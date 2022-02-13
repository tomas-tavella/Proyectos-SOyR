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

typedef struct jugador_t {
    char nombre[50];
    int mano[3];
    int cartas_levantadas[40];
    int escobas;
}jugador_t;

//******************************** Global *******************************//
int terminar = 0;
unsigned int connect_s;

void traducirCarta (char * carta, int num);

void handler(int sig);

//******************************* Main program *******************************//
int main(int argc, char *argv[]) {

    unsigned int         server_s;        // Descriptor del socket
    struct sockaddr_in   server_addr, client_addr;     // Estructuras con los datos del servidor y clientes
    struct in_addr       client_ip_addr;  // Client IP address
    int                  addr_len;        // Tamaño de las estructuras
    char                 buf_tx[1500],buf_rx[1500];
    int                  bytesrecibidos, bytesaenviar, bytestx;  // Contadores
    int                  cant_jug=1;
    int                  mazo[40];         //0 - 9 basto, 10 - 19 espada, 20 - 29 copa, 30 - 39 oro
    int                  turno=0;        // Variable para indicar el turno
    int                  suma_mano=0;
    int                  i,j,k;
    key_t                clave1, clave2, clave3;
    int                  IDmem1, IDmem2, IDmem3;
    pid_t                clientes[4];

    int *cartas_mesa = NULL;
    struct jugador_t *jugadores = NULL;
    int *contador = NULL;
    pid_t pid_padre = getpid();

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
    clave3 = ftok(PATH,NUMERO+2);
    if (clave3 == (key_t) -1){
        printf("No se pudo obtener una clave\n");
        exit(1);
    }

    // Llamar al sistema para obtener la memoria compartida
    IDmem1 = shmget(clave1, 10*sizeof(int), 0666 | IPC_CREAT); // Hasta 10 cartas en la mesa a la vez? Parece razonable, se puede cambiar -JP
    if(IDmem1 == -1){
        printf("No se pudo obtener un ID de memoria compartida\n");
        exit(2);
    }
    IDmem2 = shmget(clave2, 4*sizeof(jugador_t), 0666 | IPC_CREAT);
    if(IDmem2 == -1){
        printf("No se pudo obtener un ID de memoria compartida\n");
        exit(2);
    }
    IDmem3 = shmget(clave3, sizeof(int), 0666 | IPC_CREAT); 
    if(IDmem3 == -1){
        printf("No se pudo obtener un ID de memoria compartida\n");
        exit(2);
    }

    // Adosar el proceso a los espacios de memoria mediante un puntero
    cartas_mesa = (int *) shmat(IDmem1, (const void *)0,0);
    if (cartas_mesa == NULL){
        printf("No se pudo asociar el puntero a la memoria compartida\n");
        exit(3);
    }
    jugadores = (jugador_t *) shmat(IDmem2, (const void *)0,0);
    if (jugadores == NULL){
        printf("No se pudo asociar el puntero a la memoria compartida\n");
        exit(3);
    }
    contador = (int *) shmat(IDmem3, (const void *)0,0);
    if (contador == NULL){
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
    signal(SIGHUP,handler);
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
    *contador=0;
    addr_len = sizeof(client_addr);
    while(!terminar) { // Loop general, cuando termina una partida vuelve acá para iniciar otra
        while(turno!=cant_jug) {      // Loop de conexión de jugadores
            connect_s = accept(server_s, (struct sockaddr *)&client_addr, &addr_len);
            if (connect_s==-1) {
                perror("accept");
                return 2;
            }
            printf("Nueva conexión desde: %s:%hu , Jugador %d.\n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port),turno+1);
            if (turno==0) {
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
            }
            if ((clientes[turno]=fork())==0) { // Cada hijo se queda con su cliente
                sprintf(buf_tx,"Jugador %d de %d. Ingrese su nombre: ",turno+1,cant_jug);
                SEND_RECV();
                for (j=0;j<strlen(buf_rx);j++) if (buf_rx[j] == '\n') buf_rx[j] == '\0';
                strcpy(jugadores[turno].nombre,buf_rx);
                close(server_s);
                sprintf(buf_tx,"Esperando a los demás jugadores.");
                SEND();
                *contador+=1;
                pause();
                break;
            } else {
                turno++;
                close(connect_s);
            }
        }
        if (getppid()!=pid_padre) {     //Estamos en el padre, repartir cartas y desbloquear hijos
            int numero_aleatorio = (int)(rand() % 40);
            int primera_mano = 1, cartas_jugadas = 0;
            while(cartas_jugadas!=40){            
                // Reparto de cartas a cada jugador (3 por jugador)
                for(k=0; k<3; k++){
                    for (j=0; j<cant_jug; j++){ 
                        while(mazo[numero_aleatorio]==1){
                            numero_aleatorio = (int)(rand() % 40);
                        }
                        mazo[numero_aleatorio]=1;
                        jugadores[j].mano[k] = numero_aleatorio;
                    }
                }
                // Si es la primera mano repartir en la mesa
                if (primera_mano == 1){
                    primera_mano = 0;
                    for (j=0; j<4; j++){ 
                        while(mazo[numero_aleatorio]==1){
                            numero_aleatorio = (int)(rand() % 40);
                        }
                        mazo[numero_aleatorio]=1;
                        cartas_mesa[j]=numero_aleatorio;
                    }
                }
                // Contar cuantas cartas hay repartidas
                for(j=0; j<40; j++){
                    cartas_jugadas += mazo[j];
                }
                while(*contador!=turno){
                    
                }
                //Avisar a los hijos que se repartieron las cartas y pueden arrancar
                for (j=0; j<cant_jug; j++) {
                    kill(clientes[j],SIGHUP);
                }
            }
        } else {                        //Estamos en el hijo
            sprintf(buf_tx,"Las cartas sobre la mesa son: ");
            for (j=0;j<3;j++) {
                traducirCarta(buf_tx,cartas_mesa[j]);
                strcat(buf_tx,", ");
            }
            traducirCarta(buf_tx,cartas_mesa[3]);
            strcat(buf_tx,".\n");
            SEND();
            sprintf(buf_tx,"Tus cartas son: ");
            for (j=0;j<2;j++) {
                traducirCarta(buf_tx,jugadores[turno].mano[j]);
                strcat(buf_tx,", ");
            }
            traducirCarta(buf_tx,jugadores[turno].mano[2]);
            strcat(buf_tx,".\n");
            SEND();
            while(1);
        }
    } /* A partir de acá hay que revisar */


    /* for(int i=0;i<cant_jug;i++) {
        sprintf(buf_tx,"Tus cartas son: ");
        for (j=0;j<3;j++) {
            traducirCarta(buf_tx,mano[i][j]);
            strcat(buf_tx,", ");
        }
        strcat(buf_tx,"\n");
        SEND();
    }
    sprintf(buf_tx,"Las cartas sobre la mesa son: ");
    for(int j=0;j<4;j++) {
        traducirCarta(buf_tx,cartas_mesa[j]);
        strcat(buf_tx,", ");
    }
    strcat(buf_tx,"\n");
    for(int i=0;i<cant_jug;i++) {
        SEND();
    }
    while(1);
        
    while(turno<cant_jug && suma_mano!=120){     // Cuando el jugador descarta una carta, el numero de la carta se reemplaza por 40 en la variable mano
        // Codigo del juego
        // Avisarle al jugador correspondiente las cartas que tiene y las que estan en la mesa   
        for(int j=0; j<40; j++){                    // Contar cuantas cartas hay repartidas         
            cartas_jugadas += mazo[j];
        }
        turno++;
        if (turno-1 == cant_jug){
            suma_mano = mano[turno-1][0] + mano[turno-1][1] + mano[turno-1][2];
            turno = 0;
        }
    }
    cartas_jugadas = 0;
    for(int j=0; j<40; j++){
        cartas_jugadas += cartas_mesa[j];
    } */

        /* De acá en adelante sigue siendo código viejo */
    
    /* Mostrar puntajes */

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
}

void handler(int sig) {
}