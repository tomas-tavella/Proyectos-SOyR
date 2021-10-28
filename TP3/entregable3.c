#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>                   // Para obtener el tiempo de UNIX

// Definir puntero al archivo a utilizar
FILE *fpdat;

// Definir los parametros de ftok
#define NUMERO 30      
#define PATH "/dev/null"

// Cantidad para el tamano de la memoria compartida
#define CANTIDAD 100

// Defino los distintos semaforos

#define SEM_SYNC 0
#define SEM_READ 1
#define SEM_WRITE 2

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
    int id;
    suseconds_t tiempo;               // susesconds_t esta incluido en <sys/types.h> y devuelve el tiempo en micro segundos
    char dato[30];
}buffer;


int main(int argc, char *argv[]){
    key_t clave;
    int IDmem, IDSem;
    struct datos *memoria_comp = NULL;
    union semun argumento;
    struct timeval tiempo;
    suseconds_t tiempo_init;

    // Obtener la clave, verificando si la pudo conseguir
    clave = ftok(PATH,NUMERO);
    if (clave == (key_t) -1){
		printf("No se pudo obtener una clave\n");
		exit(1);
	}

    // Llamar al sistema para obtener la memoria compartida
    IDmem = shmget(clave, CANTIDAD*sizeof(struct datos), 0666 | IPC_CREAT);
    if(IDmem == -1){
		printf("No se pudo obtener un ID de memoria compartida\n");
		exit(2);
	}

    // Adosar el proceso al espacio de memoria mediante un puntero
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

    // Inicialización de semaforos
    argumento.val = 1; //Semaforo en verde
    semctl (IDSem, 0, SETVAL, argumento);
    argumento.val = 0; //Semaforo en rojo
    semctl (IDSem, 1, SETVAL, argumento);
    argumento.val = 0; //Semaforo en rojo
    semctl (IDSem, 2, SETVAL, argumento);

    // Verificar que el archivo exista
    fpdat = fopen("Datos.dat","rb");
    if (fpdat == 0) {
        printf("No se puede abrir el archivo.\n");
        return 0;
    }

    gettimeofday(&tiempo, NULL);                // Obtengo el tiempo de UNIX inicial, al momento que se escribe el primer dato
    tiempo_init = tiempo.tv_usec;
    
    // Se lee el archivo binario
    fread(&(buffer.dato),sizeof(struct datos),1,fpdat);
    int j=0;
    while(!feof(fpdat)){
        strcpy(memoria_comp[j].dato , buffer.dato);

        memoria_comp[j].id = j;
        
        gettimeofday(&tiempo, NULL);
        memoria_comp[j].tiempo = tiempo_init - tiempo.tv_usec;
        
        fread(&(buffer.dato),sizeof(struct datos),1,fpdat);
        j++;
    }
    fclose(fpdat);

    // Se libera la memoria compartida
    shmdt ((const void *) memoria_comp);

	shmctl (IDmem, IPC_RMID, (struct shmid_ds *)NULL);
    return 0;
}