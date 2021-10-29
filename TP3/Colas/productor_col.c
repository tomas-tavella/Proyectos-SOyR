#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>                   // Para obtener el tiempo de UNIX

// Definir puntero al archivo a utilizar
FILE *fpdat;

// Definir los parametros de ftok
#define NUMERO 30      
#define PATH "/dev/null"

// Cantidad para el tamano de la memoria compartida
#define CANTIDAD 50

// Estructura de datos a escribir en el buffer
typedef struct{
    int id;
    suseconds_t tiempo;               // susesconds_t esta incluido en <sys/types.h> y devuelve el tiempo en micro segundos
    char dato[17];
}buffer_t;

typedef struct{
	long Id_Mensaje;
	int Dato;
}mensaje_t;

int main(int argc, char *argv[]){

    key_t clave1, clave2, clave3, clave4;
    int IDmem1, IDmem2, IDmens1, IDmens2;

    buffer_t *buffer1 = NULL;
    buffer_t *buffer2 = NULL;

    struct timeval tiempo;
    struct timeval tiempo_init;

    char dato_val[17];

    mensaje_t mens;

    // Obtener las claves, verificando si la pudo conseguir
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

    // Llamar al sistema para obtener la memoria compartida y las colas de mensaje
    IDmem1 = shmget(clave1, CANTIDAD*sizeof(buffer_t), 0666 | IPC_CREAT);
    if(IDmem1 == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    IDmem2 = shmget(clave2, CANTIDAD*sizeof(buffer_t), 0666 | IPC_CREAT);
    if(IDmem2 == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    IDmens1 = msgget(clave3, 0600 | IPC_CREAT);
    if(IDmens2 == -1){
		printf("No se pudo obtener un ID de cola de mensajes\n");
		exit(2);
	}

    IDmens2 = msgget(clave4, 0600 | IPC_CREAT);
    if(IDmens2 == -1){
		printf("No se pudo obtener un ID de cola de mensajes\n");
		exit(2);
	}

    // Adosar el proceso a los espacios de memoria mediante un puntero
    buffer1 = (buffer_t *) shmat(IDmem1, (const void *)0,0);
    if (buffer1 == NULL){
		printf("No se pudo asociar el puntero a la memoria compartida\n");
		exit(3);
	}

    buffer2 = (buffer_t *) shmat(IDmem2, (const void *)0,0);
    if (buffer2 == NULL){
		printf("No se pudo asociar el puntero a la memoria compartida\n");
		exit(3);
	}

    // Verificar que el archivo exista
    fpdat = fopen("datos.dat","r");
    if (fpdat == 0) {
        printf("No se puede abrir el archivo.\n");
        return 0;
    }

    gettimeofday(&tiempo, NULL);                // Obtengo el tiempo de UNIX inicial, al momento que se escribe el primer dato
    tiempo_init = tiempo;
    // Se lee el archivo binario
    fgets(dato_val,17,fpdat);
    int i;
    int id=0;
    while(!feof(fpdat)){
        msgrcv(IDmens1,(struct msgbuf *)&mens,sizeof(mens.Dato),2,0);
        for(i=0;i<CANTIDAD;i++){
            strcpy(buffer1[i].dato,dato_val);
            buffer1[i].id=id;                                   // Asigno ID al dato, que se incrementa por cada dato que se lee
            id++;
            gettimeofday(&tiempo, NULL);
            buffer1[i].tiempo = 1000000*(tiempo.tv_sec - tiempo_init.tv_sec) + (tiempo.tv_usec - tiempo_init.tv_usec);   // Le resto el tiempo inicial al tiempo actual para obtener el timestamp
            fgets(dato_val,17,fpdat);
        }
        mens.Id_Mensaje=1;  //Tipo 1 es listo para leer, Tipo 2 es listo para escribir
        mens.Dato=1;
        if (feof(fpdat)) mens.Dato=-1;
        msgsnd(IDmens1,(struct msgbuf *)&mens,sizeof(mens.Dato),0);

        msgrcv(IDmens2,(struct msgbuf *)&mens,sizeof(mens.Dato),2,0);
        for(i=0;i<CANTIDAD;i++){
            strcpy(buffer2[i].dato,dato_val);
            buffer2[i].id=id;                                 // Asigno ID al dato, que se incrementa por cada dato que se lee
            id++;
            gettimeofday(&tiempo, NULL);
            buffer2[i].tiempo = 1000000*(tiempo.tv_sec - tiempo_init.tv_sec) + (tiempo.tv_usec - tiempo_init.tv_usec);   // Le resto el tiempo inicial al tiempo actual para obtener el timestamp
            fgets(dato_val,17,fpdat);
        }
        mens.Id_Mensaje=1;  //Tipo 1 es listo para leer, Tipo 2 es listo para escribir
        mens.Dato=1;
        if (feof(fpdat)) mens.Dato=-1;
        msgsnd(IDmens2,(struct msgbuf *)&mens,sizeof(mens.Dato),0);
    }
    fclose(fpdat);

    // Se libera la memoria compartida
    shmdt ((const void *) buffer1);
    shmdt ((const void *) buffer2);

    return 0;
}