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

//------------------------------------------------ Defines ------------------------------------------------//

#define  PORT_NUM 1234  // Numero de Port
#define  IP_ADDR "127.0.0.1" // Direccion IP LOCALHOST
#define NCONCUR 4
#define SOCKET_PROTOCOL 0

#define NUMERO 30
#define MAX_CARTAS_MESA 40      
#define PATH "/dev/null"

#define ROJO "\033[1;31m"
#define VERDE "\033[1;32m"
#define AMARILLO "\033[1;33m"
#define CYAN "\033[1;36m"
#define BLANCO "\033[0m"
#define BOLD "\033[1m"

#define SEND_RECV() {\
    bytesaenviar = strlen(buf_tx);\
    bytestx = send(connect_s, buf_tx, bytesaenviar, 0);\
    bytesrecibidos=recv(connect_s, buf_rx, sizeof(buf_rx), 0);\
}

#define SEND() {\
    bytesaenviar = strlen(buf_tx);\
    bytestx = send(connect_s, buf_tx, bytesaenviar, 0);\
}

typedef struct jugador_t{
    char nombre[50];
    int mano[3];
    int cartas_levantadas[40];
    int cant_cartas;
    int escobas;
}jugador_t;

typedef struct jugada_t{
    int cartas[10];
    int posiciones[10];
    char op;
    int cant_cartas;
}jugada_t;

typedef struct{
    long tipo;
    int turno;
    char op;
}mensaje_t;

//---------------------------------------------- Global ----------------------------------------------//

int terminar = 0;
unsigned int connect_s;

void traducirCarta (char * carta, int num);

void carta_mesa (char *buf_tx , int suma_mesa);

void carta_mano (char *buf_tx , int suma_mano);

int cmpfunc (const void * a, const void * b);

void handler(int sig);

//---------------------------------------------- Main program ----------------------------------------------//

int main(int argc, char *argv[]) {
    
    //------------------------------------------------------------------------------------------------------//
    //-------------------------------------- Declaracion de variables --------------------------------------//
    //------------------------------------------------------------------------------------------------------//

    unsigned int         server_s;        // Descriptor del socket
    struct sockaddr_in   server_addr, client_addr;     // Estructuras con los datos del servidor y clientes
    struct in_addr       client_ip_addr;  // Client IP address
    int                  addr_len;        // Tamaño de las estructuras
    char                 buf_tx[1500],buf_rx[1500];
    int                  bytesrecibidos, bytesaenviar, bytestx;  // Contadores
    int                  cant_jug=1;
    int                  mazo[40];         //0 - 9 basto, 10 - 19 espada, 20 - 29 copa, 30 - 39 oro
    int                  turno=0;        // Variable para indicar el turno
    int                  suma_mano=0, suma_mesa=0;
    int                  i,j,k,l;
    key_t                clave1, clave2, clave3, clave4, clave5;
    int                  IDmem1, IDmem2, IDmem3, HaP, PaH;
    pid_t                clientes[4];
    int                  no_valido;
    int                  ultimoLevante = 4;
    mensaje_t            mensaje;
    int                  ronda = 1;
    char                 letras_seleccion ='a';

    int *cartas_mesa = NULL;
    struct jugador_t *jugadores = NULL;
    struct jugada_t *jugada = NULL;
    pid_t pid_padre = getpid();

    for (i=0;i<=39;i++) mazo[i]=0;      // Inicializo mazo en 0
    i = 0;

    //------------------------------------------------------------------------------------------------------//
    //---------------------------- Setup de memoria comparitda y colas de mensajes -------------------------//
    //------------------------------------------------------------------------------------------------------//

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
    clave4 = ftok(PATH,NUMERO+3);
    if (clave4 == (key_t) -1){
        printf("No se pudo obtener una clave\n");
        exit(1);
    }
    clave5 = ftok(PATH,NUMERO+4);
    if (clave5 == (key_t) -1){
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
    IDmem3 = shmget(clave3, sizeof(jugada_t), 0666 | IPC_CREAT); 
    if(IDmem3 == -1){
        printf("No se pudo obtener un ID de memoria compartida\n");
        exit(2);
    }
    HaP = msgget(clave4, 0600 | IPC_CREAT);
    if(HaP == -1){
        printf("No se pudo obtener un ID de cola de mensajes\n");
        exit(2);
    }
    PaH = msgget(clave5, 0600 | IPC_CREAT);
    if(PaH == -1){
        printf("No se pudo obtener un ID de cola de mensajes\n");
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
    jugada = (jugada_t *) shmat(IDmem3, (const void *)0,0);
    if (jugada == NULL){
        printf("No se pudo asociar el puntero a la memoria compartida\n");
        exit(3);
    }

    //----------------------------------------------------------------------------------------------------------------------//
    //---------------------------------------- Creación del socket para el servidor ----------------------------------------//
    //----------------------------------------------------------------------------------------------------------------------//

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

    //--------------------------------------------------------------------------------------------------------------------------------------//
    //----------------------------------------- Creacion de la partida y conexion de los jugadores -----------------------------------------//
    //--------------------------------------------------------------------------------------------------------------------------------------//

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
                while(!(buf_rx[0] >= '2' && buf_rx[0] <= '4')){
                    sprintf(buf_tx,"Ingrese una opción válida: \n");
                    SEND_RECV();
                }
                cant_jug = atoi(buf_rx);
            }
            if ((clientes[turno]=fork())==0) { // Cada hijo se queda con su cliente
                sprintf(buf_tx,"Jugador %d de %d. Ingrese su nombre: ",turno+1,cant_jug);
                SEND_RECV();
                if (buf_rx[0] == '\n') sprintf(jugadores[turno].nombre,"Jugador %d",turno+1);
                for (j=0;j<strlen(buf_rx);j++) if (buf_rx[j] == '\n') buf_rx[j] = '\0';
                strcpy(jugadores[turno].nombre,buf_rx);
                jugadores[turno].cant_cartas=0;
                jugadores[turno].escobas=0;
                close(server_s);
                sprintf(buf_tx,"Esperando a los demás jugadores.\n");
                SEND();
                mensaje.tipo=1;
                mensaje.turno=turno;
                mensaje.op='L';
                msgsnd(HaP,(struct msgbuf *)&mensaje,sizeof(mensaje)-sizeof(long),0);
                break;
            } else {
                turno++;
                close(connect_s);
            }
        }

        //--------------------------------------------------------------------------------------------------------------//
        //--------------------------------------------- Seccion del padre ----------------------------------------------//
        //--------------------------------------------------------------------------------------------------------------//

        if (getppid()!=pid_padre) {     //Estamos en el padre, repartir cartas y desbloquear hijos
            int numero_aleatorio = (int)(rand() % 40);
            int primera_mano = 1, cartas_jugadas = 0;
            for (j=0; j<10; j++){ //Inicializo la mesa sin cartas
                cartas_mesa[j]=40; 
            }
            printf("Esperando a los jugadores.\n");
            for (j=0; j<cant_jug; j++) {    //Se sale de acá cuando llegan los mensajes de todos los jugadores
                msgrcv(HaP,(struct msgbuf *)&mensaje,sizeof(mensaje)-sizeof(long),1,0);
                printf("Jugador %d listo.\n",j);
            }
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
                cartas_jugadas=0;
                for(j=0; j<40; j++){
                    cartas_jugadas += mazo[j];
                }
                printf("Arrancamos. Ronda %d.\n",ronda);
                ronda++;
                //Avisar a los hijos que se repartieron las cartas y pueden arrancar
                for (i=0; i<3; i++){
                    for (j=0; j<cant_jug; j++) {
                        mensaje.turno=j;
                        mensaje.op='A';
                        for (k=0; k<cant_jug; k++) {
                            mensaje.tipo=k+1;
                            msgsnd(PaH,(struct msgbuf *)&mensaje,(size_t) 5,0);
                        }
                        mensaje.op='T';
                        for (k=0; k<cant_jug; k++) {
                            mensaje.tipo=k+1;
                            msgsnd(PaH,(struct msgbuf *)&mensaje,(size_t) 5,0);
                        }
                        msgrcv(HaP,(struct msgbuf *)&mensaje,(size_t) 5,1,0);
                        if(mensaje.op=='L'){
                            ultimoLevante=mensaje.turno;
                            printf("Ultimo levante: %d\n",ultimoLevante);
                        }
                        for (k=0; k<cant_jug; k++) {
                            mensaje.tipo=k+1;
                            msgsnd(PaH,(struct msgbuf *)&mensaje,(size_t) 5,0);
                        }
                    }
                }
            }
            
            j=0;
            while (cartas_mesa[j]!=40){
                jugadores[ultimoLevante].cartas_levantadas[jugadores[ultimoLevante].cant_cartas]=cartas_mesa[j];
                jugadores[ultimoLevante].cant_cartas++;
                j++;
            }
            mensaje.op='F';
            for (k=0;k<cant_jug;k++) qsort(jugadores[k].cartas_levantadas,(size_t) 40,sizeof(int),cmpfunc);
            for(j=0;j<cant_jug;j++){
                mensaje.tipo=j+1;    
                mensaje.turno=0;    
                msgsnd(PaH,(struct msgbuf *)&mensaje,(size_t) 5,0);
            }
            turno=0;
            cant_jug=1;


        //-----------------------------------------------------------------------------------------------------------------//
        //--------------------------------------------- Seccion de los hijos ----------------------------------------------//
        //-----------------------------------------------------------------------------------------------------------------//

        } else {                        //Estamos en el hijo, esperar operacion en mensaje
            for(j=0;j<40;j++){
                jugadores[turno].cartas_levantadas[j]=40;
            }
            do {
                msgrcv(PaH,(struct msgbuf *)&mensaje,(size_t) 5,turno+1,0);
                printf("%d: Recibí %c.\n",getpid(),mensaje.op);
                switch (mensaje.op){
                    case 'A':
                        suma_mesa=0;
                        for(k=0;k<10;k++){
                            if(cartas_mesa[k]!=40){
                                suma_mesa++;
                            }
                        }
                        if (suma_mesa!=0) {
                            sprintf(buf_tx,"\nLas cartas sobre la mesa son: ");
                            letras_seleccion='a';
                            for (j=0;j<suma_mesa-1;j++) {
                                strcat(buf_tx,"\033[1m(");
                                strcat(buf_tx,&letras_seleccion);
                                strcat(buf_tx,")\033[0m ");
                                traducirCarta(buf_tx,cartas_mesa[j]);
                                strcat(buf_tx,", ");
                                letras_seleccion++;
                            }
                            strcat(buf_tx,"\033[1m(");
                            strcat(buf_tx,&letras_seleccion);
                            strcat(buf_tx,")\033[0m ");
                            traducirCarta(buf_tx,cartas_mesa[suma_mesa-1]);
                            strcat(buf_tx,".\n");
                            SEND();
                        } else {
                            sprintf(buf_tx,"\nNo quedan cartas sobre la mesa.\n");
                            SEND();
                        }
                        if (jugadores[turno].mano[0]==40){
                            sprintf(buf_tx,"No quedan cartas en tu mano");
                        }else{
                            sprintf(buf_tx,"Tus cartas son: ");
                            suma_mano=0;
                            for(int k=0;k<3;k++){
                                if(jugadores[turno].mano[k]!=40){ 
                                    suma_mano++;
                                }
                            }
                            letras_seleccion='a';
                            for (j=0;j<suma_mano-1;j++) {
                                strcat(buf_tx,"\033[1m(");
                                strcat(buf_tx,&letras_seleccion);
                                strcat(buf_tx,")\033[0m ");
                                traducirCarta(buf_tx,jugadores[turno].mano[j]);
                                strcat(buf_tx,", ");
                                letras_seleccion++;
                            }
                            strcat(buf_tx,"\033[1m(");
                                strcat(buf_tx,&letras_seleccion);
                                strcat(buf_tx,")\033[0m ");
                            traducirCarta(buf_tx,jugadores[turno].mano[suma_mano-1]);   
                        } 
                        strcat(buf_tx,".\n\n");
                        SEND();
                        if (mensaje.turno!=turno) {                       
                            sprintf(buf_tx,"\033[1m\n------------------------------------ Espero la jugada de %s ------------------------------------\033[0m\n",jugadores[mensaje.turno].nombre);
                            SEND();
                        } else {
                            sprintf(buf_tx,"\033[1m\n------------------------------------ Es tu turno, %s ------------------------------------\033[0m\n",jugadores[turno].nombre);
                            SEND();
                        }
                        break;
                    case 'T':
                        if (mensaje.turno==turno) {
                            do{
                                int eleccion_mesa[9];
                                int suma_jugada = 0;
                                suma_mesa=0;
                                for(k=0;k<10;k++){
                                    if(cartas_mesa[k]!=40){
                                        suma_mesa++;
                                    }
                                }
                                if (suma_mesa!=0) {
                                    sprintf(buf_tx,"\nLas cartas sobre la mesa son: ");
                                    letras_seleccion = 'a';
                                    for (j=0;j<suma_mesa-1;j++) {
                                        strcat(buf_tx,"\033[1m(");
                                        strcat(buf_tx,&letras_seleccion);
                                        strcat(buf_tx,")\033[0m ");
                                        traducirCarta(buf_tx,cartas_mesa[j]);
                                        strcat(buf_tx,", ");
                                        letras_seleccion++;
                                    }
                                    strcat(buf_tx,"\033[1m(");
                                    strcat(buf_tx,&letras_seleccion);
                                    strcat(buf_tx,")\033[0m ");
                                    traducirCarta(buf_tx,cartas_mesa[suma_mesa-1]);
                                    strcat(buf_tx,".\n");
                                    SEND();
                                }

                                if (jugadores[turno].mano[0]==40){
                                    sprintf(buf_tx,"No quedan cartas en tu mano");
                                }else{
                                    sprintf(buf_tx,"Tus cartas son: ");
                                    suma_mano=0;
                                    for(int k=0;k<3;k++){
                                        if(jugadores[turno].mano[k]!=40){ 
                                            suma_mano++;
                                        }
                                    }
                                    letras_seleccion = 'a';
                                    for (j=0;j<suma_mano-1;j++) {
                                        strcat(buf_tx,"\033[1m(");
                                        strcat(buf_tx,&letras_seleccion);
                                        strcat(buf_tx,")\033[0m ");
                                        traducirCarta(buf_tx,jugadores[turno].mano[j]);
                                        strcat(buf_tx,", ");
                                        letras_seleccion++;
                                    }
                                    strcat(buf_tx,"\033[1m(");
                                    strcat(buf_tx,&letras_seleccion);
                                    strcat(buf_tx,")\033[0m ");
                                    traducirCarta(buf_tx,jugadores[turno].mano[suma_mano-1]);   
                                }
                                strcat(buf_tx,".\n\n");
                                SEND();
                    
                                if(suma_mesa!=0){   
                                    sprintf(buf_tx,"¿Levanta o descarta una carta? (L) (D)\n");
                                    SEND_RECV();
                                    while (buf_rx[0] != 'L' && buf_rx[0] != 'D'){
                                        sprintf(buf_tx,"Ingrese una opción válida: \n");
                                        SEND_RECV();
                                    }
                                }else{
                                    sprintf(buf_tx,"No quedan mas cartas sobre la mesa, tenes que descartar\n");
                                    SEND();
                                    strcpy(buf_rx,"D");
                                }
                                jugada->op = buf_rx[0];
                                suma_mano=0;
                                for(k=0;k<3;k++){                   //Cuenta las cartas que tiene el jugador en la mano y dependiendo de eso se envia un mensaje determinado
                                    if(jugadores[turno].mano[k]!=40){ 
                                        suma_mano++;
                                    }
                                }

                                for (k=0;k<10;k++) jugada->cartas[k]=40;

                                carta_mano(buf_tx,suma_mano);
                                SEND_RECV();
                        
                                while (!(buf_rx[0] >= 'a' && buf_rx[0] <= ('a' + suma_mano-1))){
                                    sprintf(buf_tx,"Ingrese una opción válida: \n");
                                    SEND_RECV();
                                }
                                // Switch que decide que hace si descarta o levanta
                                switch (jugada->op) {
                                    case 'L':
                                        jugada->posiciones[0]=buf_rx[0]-'a';
                                        jugada->cartas[0]=jugadores[turno].mano[buf_rx[0]-'a'];
                                        suma_mesa=0;
                                        for(k=0;k<10;k++){                   //Cuenta las cartas que hay en mesa y dependiendo de eso se envia un mensaje determinado
                                            if(cartas_mesa[k]!=40){
                                                suma_mesa++;
                                            }
                                        }
                                        carta_mesa(buf_tx,suma_mesa);
                                        SEND_RECV();
                                        while (!(buf_rx[0] >= 'a' && buf_rx[0] <= ('a' + suma_mesa-1))){ 
                                            sprintf(buf_tx,"Ingrese una opción válida: \n");
                                            SEND_RECV();
                                        }
                                        jugada->posiciones[1]=buf_rx[0]-'a';
                                        jugada->cartas[1]=cartas_mesa[buf_rx[0]-'a'];
                                        eleccion_mesa[0] = buf_rx[0];                   // Se guarda que cartas de la mesa ya fueron elegidas
                                        suma_jugada = jugadores[turno].mano[jugada->posiciones[0]]%10 + cartas_mesa[jugada->posiciones[1]]%10 + 2;
                                        sprintf(buf_tx,"Las cartas elegidas suman %d.\n",suma_jugada); 
                                        SEND();
                                        j=2;
                                        jugada->cartas[j] = 40;
                                        while(suma_jugada<15 && suma_mesa!=0){
                                    
                                            suma_mesa=0;
                                            for(k=0;k<10;k++){                   //Cuenta las cartas que hay en mesa y dependiendo de eso se envia un mensaje determinado
                                                if(cartas_mesa[k]!=40){
                                                    suma_mesa++;
                                                }
                                            }
                                            carta_mesa(buf_tx,suma_mesa);
                                            SEND_RECV();
                                            while ( 
                                                (!(buf_rx[0] >= 'a' && buf_rx[0] <= ('a' + suma_mesa-1)))
                                                || buf_rx[0]==eleccion_mesa[0] || buf_rx[0]==eleccion_mesa[1]
                                                || buf_rx[0]==eleccion_mesa[2] || buf_rx[0]==eleccion_mesa[3]
                                                || buf_rx[0]==eleccion_mesa[4] || buf_rx[0]==eleccion_mesa[5]
                                                || buf_rx[0]==eleccion_mesa[6] || buf_rx[0]==eleccion_mesa[7]
                                                || buf_rx[0]==eleccion_mesa[8]
                                            ){
                                                sprintf(buf_tx,"Ingrese una opción válida: \n");
                                                SEND_RECV();
                                            }
                                            jugada->posiciones[j]=buf_rx[0]-'a';
                                            jugada->cartas[j]=cartas_mesa[buf_rx[0]-'a'];
                                            eleccion_mesa[j-1] = buf_rx[0];                 // Se guarda que cartas de la mesa ya fueron elegidas
                                            suma_jugada += (cartas_mesa[jugada->posiciones[j]]%10 + 1);
                                            sprintf(buf_tx,"Las cartas elegidas suman %d.\n",suma_jugada);
                                            SEND();

                                            suma_mesa=0;
                                            for(k=0;k<10;k++){
                                                if(cartas_mesa[k]!=40){
                                                    suma_mesa++;
                                                }
                                            }
                                            j++;
                                        }

                                        if(suma_jugada==15){
                                            jugadores[turno].cartas_levantadas[jugadores[turno].cant_cartas]=jugadores[turno].mano[jugada->posiciones[0]];
                                            jugadores[turno].cant_cartas++;
                                            jugadores[turno].mano[jugada->posiciones[0]] = 40;
                                            for(k=1; k<j; k++){
                                                jugadores[turno].cartas_levantadas[jugadores[turno].cant_cartas]=cartas_mesa[jugada->posiciones[k]];
                                                jugadores[turno].cant_cartas++;
                                                cartas_mesa[jugada->posiciones[k]] = 40;
                                            }
                                            qsort(jugadores[turno].mano,(size_t) 3,sizeof(int),cmpfunc);     // Funcion para ordenar la mano de menor a mayor (los espacios vacios quedan al final)
                                            qsort(cartas_mesa,(size_t) 10,sizeof(int),cmpfunc);     // Funcion para ordenar la mano de menor a mayor (los espacios vacios quedan al final)
                                            no_valido=0;
                                            for (i=0; i<9; i++) eleccion_mesa[i]=0;
                                            suma_mesa=0;
                                            for(k=0;k<10;k++){              //Agregue 
                                                if(cartas_mesa[k]!=40){
                                                    suma_mesa++;
                                                }
                                            }
                                            if (suma_mesa==0) {
                                                jugadores[turno].escobas++;
                                                jugada->op='E';
                                            }
                                            mensaje.op='L';
                                            mensaje.turno=turno;
                                            mensaje.tipo=1;

                                            /////////////Aca
                                            msgsnd(HaP,(struct msgbuf *)&mensaje,(size_t) 5,0);
                                        }else{
                                            strcpy(buf_tx,"Las cartas elegidas no suman 15.\n");
                                            SEND();
                                            for (i=0; i<9; i++) eleccion_mesa[i]=0;
                                            no_valido = 1;
                                        }
                                        break;
                                    case 'D':
                                        // Busco la posicion de la ultima carta de la mesa
                                        j=0;
                                        while(cartas_mesa[j]!=40){
                                            j++;
                                        }
                                        cartas_mesa[j]=jugadores[turno].mano[buf_rx[0]-'a'];
                                        suma_mesa++;
                                        jugada->cartas[0]=jugadores[turno].mano[buf_rx[0]-'a'];
                                        jugadores[turno].mano[buf_rx[0]-'a'] = 40;
                                        qsort(jugadores[turno].mano,(size_t) 3,sizeof(int),cmpfunc);     // Funcion para ordenar la mano de menor a mayor (los espacios vacios quedan al final)
                                        no_valido=0;
                                        mensaje.op='D';
                                        mensaje.turno=turno;
                                        mensaje.tipo=1;
                                        msgsnd(HaP,(struct msgbuf *)&mensaje,(size_t) 5,0);
                                        break;
                                    default:
                                        break;
                                }
                            } while(no_valido);
                        }
                        break;
                    case 'L':
                        sprintf(buf_tx,"\n%s levantó ",jugadores[mensaje.turno].nombre);
                        j=0;
                        while (jugada->cartas[j]!=40) {
                            traducirCarta(buf_tx,jugada->cartas[j]);
                            strcat(buf_tx,", ");
                            j++;
                        }
                        char *coma_final;                   // Reemplazo la ultima coma por punto
                        coma_final = strrchr(buf_tx,',');
                        strcpy(coma_final,".");
                        if (jugada->op=='E') strcat(buf_tx," ESCOBA!");
                        strcat(buf_tx,"\n");
                        SEND();
                        break;
                    case 'D':
                        sprintf(buf_tx,"\n%s descartó ",jugadores[mensaje.turno].nombre);
                        traducirCarta(buf_tx,jugada->cartas[0]);
                        strcat(buf_tx,".\n");
                        SEND();
                        break;
                    default:
                        break;
                }
            } while(mensaje.op!='F');            
            for(k=0;k<cant_jug;k++){
                j=0;
                sprintf(buf_tx,"Las cartas de %s son: \n", jugadores[k].nombre);
                for (l=1;l<=4;l++){
                    while (jugadores[k].cartas_levantadas[j]<10*l){
                        traducirCarta(buf_tx,jugadores[k].cartas_levantadas[j]);
                        strcat(buf_tx,"\t\t");
                        j++;
                    }
                    strcat(buf_tx,"\n");
                }
                SEND();

                sprintf(buf_tx,"Tiene %d cartas\nHizo %d escobas\n",jugadores[k].cant_cartas ,jugadores[k].escobas);
                strcat(buf_tx,".\n");
                SEND();
            }
            shmdt((const void *) cartas_mesa);
            shmdt((const void *) jugadores);
            shmdt((const void *) jugada);
            return 0;
        }

    //--------------------------------------------------------------------------------------------------------------//
    //---------------------------- Cierre y eliminacion de memoria compartida y colas ------------------------------//
    //--------------------------------------------------------------------------------------------------------------//

    }
    for (i=0; i<cant_jug; i++) {
        wait(NULL);
    }
    shmdt((const void *) cartas_mesa);
    shmdt((const void *) jugadores);
    shmdt((const void *) jugada);
    shmctl(IDmem1, IPC_RMID, (struct shmid_ds *)NULL);
    shmctl(IDmem2, IPC_RMID, (struct shmid_ds *)NULL);
    shmctl(IDmem3, IPC_RMID, (struct shmid_ds *)NULL);
    msgctl(PaH, IPC_RMID, (struct msqid_ds *)NULL);
    msgctl(HaP, IPC_RMID, (struct msqid_ds *)NULL);
}


//------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------- Funciones ----------------------------------------------------//
//------------------------------------------------------------------------------------------------------------------//

                
void traducirCarta (char * carta, int num) {
    //Función para traducir el número en el nombre completo de la carta
    int aux = num / 10;
    switch (aux) {
        case 0:     // Bastos
            strcat(carta,VERDE);
            break;
        case 1:     // Espadas
            strcat(carta,CYAN);
            break;
        case 2:     // Copas
            strcat(carta,ROJO);
            break;
        case 3:     // Oros
            strcat(carta,AMARILLO);
            break;
        default:
            break;
    }    
    
    aux = num % 10;
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

    strcat(carta,BLANCO);

    return;
}

void carta_mesa (char *buf_tx , int suma_mesa){
    switch (suma_mesa) {
        case 1:
            sprintf(buf_tx,"Carta sobre la mesa? (a)\n");
            break;
        case 2:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b)\n");
            break;
        case 3:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b,c)\n");
            break;
        case 4:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b,c,d)\n");
            break;
        case 5:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b,c,d,e)\n");
            break;
        case 6:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b,c,d,e,f)\n");
            break;
        case 7:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b,c,d,e,f,g)\n");
            break;
        case 8:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b,c,d,e,f,g,h)\n");
            break;
        case 9:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b,c,d,e,f,g,h,i)\n");
            break;
        case 10:
            sprintf(buf_tx,"Carta sobre la mesa? (a,b,c,d,e,f,g,h,i,j)\n");
            break;
        default:
            break;
    }
}

void carta_mano (char *buf_tx , int suma_mano){
    switch (suma_mano) {
        case 1:
            sprintf(buf_tx,"Tu carta (a)\n");
            break;
        case 2:
            sprintf(buf_tx,"Tu carta (a,b)\n");
            break;
        case 3:
            sprintf(buf_tx,"Tu carta (a,b,c)\n");
            break;
        default:
            break;
    }
}

void handler(int sig) {
    if (sig==SIGHUP) {
	    terminar=1;
	    printf("Señal HUP recibida, terminando con la próxima partida.\n");
	}
}

int cmpfunc (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}