#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>

// Definir puntero al archivo a utilizar
FILE *fpcsv;

// Definir los parametros de ftok
#define NUMERO 30      
#define PATH "/dev/null"

// Cantidad para el tamano de la memoria compartida
#define CANTIDAD 50

// Estructura de datos a escribir en el buffer
typedef struct{
    int id;
    suseconds_t tiempo;               // susesconds_t esta incluido en <sys/types.h> y devuelve el tiempo en micro segundos
    float dato;
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

    buffer_t aux;

    struct timeval tiempo;
    suseconds_t tiempo_init;

    float dato_val;

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

    // Llamar al sistema para obtener la memoria compartida
    IDmem1 = shmget(clave1, CANTIDAD*sizeof(buffer_t), 0666);
    if(IDmem1 == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    IDmem2 = shmget(clave2, CANTIDAD*sizeof(buffer_t), 0666);
    if(IDmem2 == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    IDmens1 = msgget(clave3, 0600);
    if(IDmens2 == -1){
		printf("No se pudo obtener un ID de cola de mensajes\n");
		exit(2);
	}

    IDmens2 = msgget(clave4, 0600);
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

    mens.Id_Mensaje=2;
    mens.Dato=1;
    msgsnd(IDmens1,(struct msgbuf *)&mens,sizeof(mens.Dato),0);
    msgsnd(IDmens2,(struct msgbuf *)&mens,sizeof(mens.Dato),0);

    // Verificar que se pueda crear el archivo
    fpcsv = fopen("datos.csv","w");
    if (fpcsv == 0) {
        printf("No se puede crear el archivo.\n");
        return 0;
    }
    int i;
    while(1){
        msgrcv(IDmens1,(struct msgbuf *)&mens,sizeof(mens.Dato),1,0);
        if (mens.Dato==-1) break;
        for(i=0;i<CANTIDAD;i++){
            aux=buffer1[i];
            printf("%d,%ld us,%f\n",aux.id,aux.tiempo,aux.dato);
            fprintf(fpcsv,"%d,%ld us,%f\n",aux.id,aux.tiempo,aux.dato);
        }
        mens.Id_Mensaje=2;  //Tipo 1 es listo para leer, Tipo 2 es listo para escribir
        mens.Dato=1;
        msgsnd(IDmens1,(struct msgbuf *)&mens,sizeof(mens.Dato),0);

        msgrcv(IDmens2,(struct msgbuf *)&mens,sizeof(mens.Dato),1,0);
        if (mens.Dato==-1) break;
        for(i=0;i<CANTIDAD;i++){
            aux=buffer2[i];
            printf("%d,%ld us,%f\n",aux.id,aux.tiempo,aux.dato);
            fprintf(fpcsv,"%d,%ld us,%f\n",aux.id,aux.tiempo,aux.dato);
        }
        mens.Id_Mensaje=2;  //Tipo 1 es listo para leer, Tipo 2 es listo para escribir
        mens.Dato=1;
        msgsnd(IDmens2,(struct msgbuf *)&mens,sizeof(mens.Dato),0);
    }

    fclose(fpcsv);

    shmdt ((const void *) buffer1);
    shmdt ((const void *) buffer2);

    shmctl (IDmem1, IPC_RMID, (struct shmid_ds *)NULL);
    shmctl (IDmem2, IPC_RMID, (struct shmid_ds *)NULL);

    msgctl (IDmens1, IPC_RMID, (struct msqid_ds *)NULL);
    msgctl (IDmens2, IPC_RMID, (struct msqid_ds *)NULL);

    return 0;
}