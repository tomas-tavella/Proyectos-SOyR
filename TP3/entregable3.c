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


int main(int argc, char *argv[]){
    // Codigo
    key_t clave;
    int id_memoria;

    clave = ftok(ARCHIVO,NUMERO);
    if (Clave == (key_t)-1){
		printf("No consegui  clave para memoria compartida\n");
		exit(1);
	}

    
    return 0;
}