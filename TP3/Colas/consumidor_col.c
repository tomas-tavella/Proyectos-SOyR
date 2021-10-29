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
#define CANTIDAD 100

// Estructura de datos a escribir en el buffer
struct datos{
    int id;
    suseconds_t tiempo;               // susesconds_t esta incluido en <sys/types.h> y devuelve el tiempo en micro segundos
    char dato[30];
}buffer;

int main(int argc, char *argv[]){
    key_t clave1, clave2, clave3, clave4;
    int IDmem1, IDmem2, IDmens1, IDmens2;
    struct datos *memoria_comp1 = NULL;
    struct datos *memoria_comp2 = NULL;

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

    clave4 = ftok(PATH,NUMERO+3;
    if (clave4 == (key_t) -1){
		printf("No se pudo obtener una clave\n");
		exit(1);
	}

    // Llamar al sistema para obtener la memoria compartida
    IDmem1 = shmget(clave1, CANTIDAD*sizeof(struct datos), 0666 | IPC_CREAT);
    if(IDmem1 == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    IDmem2 = shmget(clave2, CANTIDAD*sizeof(struct datos), 0666 | IPC_CREAT);
    if(IDmem2 == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    IDmens1 = msgget(clave3, 0666 | IPC_CREAT);
    if(IDmens2 == -1){
		printf("No se pudo obtener un ID de cola de mensajes\n");
		exit(2);
	}

    IDmens2 = msgget(clave4, 0666 | IPC_CREAT);
    if(IDmens2 == -1){
		printf("No se pudo obtener un ID de cola de mensajes\n");
		exit(2);
	}

    // Adosar el proceso a los espacios de memoria mediante un puntero
    memoria_comp1 = (struct datos *) shmat(IDmem1, (const void *)0,0);
    if (memoria_comp1 == NULL){
		printf("No se pudo asociar el puntero a la memoria compartida\n");
		exit(3);
	}

    memoria_comp2 = (struct datos *) shmat(IDmem2, (const void *)0,0);
    if (memoria_comp2 == NULL){
		printf("No se pudo asociar el puntero a la memoria compartida\n");
		exit(3);
	}

    // Verificar que se pueda crear el archivo
    fpcsv = fopen("datos.csv","w");
    if (fpcsv == 0) {
        printf("No se puede crear el archivo.\n");
        return 0;
    }


    return 0;
}