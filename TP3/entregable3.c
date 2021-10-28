#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Definir puntero al archivo a utilizar
FILE *fpdat;

// Definir los parametros de ftok
#define NUMERO 30      
#define PATH "/dev/null"

// Cantidad para el tamano de la memoria compartida
#define CANTIDAD 100

// Definir las operaciones para los semaforos
#define BLOQUEAR(OP) ((OP).sem_op = -1)             // OP es una estructura
#define DESBLOQUEAR(OP) ((OP).sem_op = +1)

// Crear la union necesaria para semctl (Linux-specific)
// Union es como struct, pero solo uno de los elementos puede tomar una valor a la vez
union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf;
};

// Estructura de datos a escribir en el buffer
struct datos{
    unsigned short id;
    suseconds_t tiempo;               // susesconds_t esta incluido en <sys/types.h> y devuelve el tiempo en micro segundos
    char dato[30];
}buffer;


int main(int argc, char *argv[]){
    key_t clave;
    int IDmem, IDSem;
    struct datos *memoria_comp = NULL;
    union semun argumento;

    // Obtener la clave, verificando si la pudo conseguir
    clave = ftok(PATH,NUMERO);
    if (clave == (key_t) -1){
		printf("No se pudo obtener una clave\n");
		exit(1);
	}

    // Llamar al sistema para obtener la memoria compartida
    IDmem = shmget(clave, CANTIDAD*sizeof(buffer), 0666 | IPC_CREAT);
    if(IDmem == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    // Adosar el proceso al espacio de memoria mediante un puntero
    //memoria_comp = (struct datos *) shmget(clave, (void *) NULL, NULL);
    memoria_comp = (struct datos *) shmat(IDmem, (const void *)0,0);
    if (memoria_comp == NULL){
		printf("No se pudo asociar el puntero a la memoria compartida\n");
		exit(3);
	}

    // Creación de semaforos
    IDSem = semget(clave, 3, 0666 | IPC_CREAT);
    if (IDSem == -1){
        printf("No se puede crear el semáforo\n");
        exit(4);
    }

    //Inicialización de semaforos
    argumento.val = 1; //Semaforo en verde
    semctl (IDSem, 0, SETVAL, argumento);
    argumento.val = 0; //Semaforo en rojo
    semctl (IDSem, 1, SETVAL, argumento);
    argumento.val = 0; //Semaforo en rojo
    semctl (IDSem, 2, SETVAL, argumento);

    fclose(fpdat);
    return 0;
}