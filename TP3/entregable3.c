#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Definir los parametros de ftok
#define NUMERO 30               // Grupo 30 tuki :basa:
#define ARCHIVO "/dev/null"

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

struct datos{
    unsigned short id;
    float tiempo; //Este no se que ponerle
    char dato[30];
}


int main(int argc, char *argv[]){
    key_t clave;
    int IDmem;

    // Obtener la clave, verificando si la pudo conseguir
    clave = ftok(ARCHIVO,NUMERO);
    if (Clave == (key_t) -1){
		printf("No consegui  clave para memoria compartida\n");
		exit(1);
	}

    // Llamar al sistema para obtener la memoria compartida
    IDmem = shmget(clave, CANTIDAD*sizeof(datos), 0666 | IPC_CREAT);
    if(IDmem == -1){
		printf("No consegui ID para memoria compartida\n");
		exit (2);
	}
    return 0;
}