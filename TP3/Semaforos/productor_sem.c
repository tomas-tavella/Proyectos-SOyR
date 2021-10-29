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
#define NUMERO1 30 
#define NUMERO2 31   
#define PATH "/dev/null"

// Cantidad para el tamano de la memoria compartida
#define CANTIDAD 50

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
    float dato;
}buffer;


int main(int argc, char *argv[]){
    key_t clave1, clave2;
    int IDmem1, IDmem2, IDsem;
    struct datos *memoria_comp1 = NULL;
    struct datos *memoria_comp2 = NULL;
    union semun argumento;
    struct timeval tiempo;
    struct sembuf op;
    suseconds_t tiempo_init;

    // Obtener la clave, verificando si la pudo conseguir
    clave1 = ftok(PATH,NUMERO1);
    if (clave1 == (key_t) -1){
		printf("No se pudo obtener la primera clave\n");
		exit(1);
	}
    clave2 = ftok(PATH,NUMERO2);
    if (clave2 == (key_t) -1){
		printf("No se pudo obtener la segunda clave\n");
		exit(2);
	}

    // Llamar al sistema para obtener la memoria compartida
    IDmem1 = shmget(clave1, CANTIDAD*sizeof(struct datos), 0666 | IPC_CREAT);
    if(IDmem1 == -1){
		printf("No se pudo obtener un ID de la primera memoria compartida\n");
		exit(3);
	}
    IDmem2 = shmget(clave2, CANTIDAD*sizeof(struct datos), 0666 | IPC_CREAT);
    if(IDmem2 == -1){
		printf("No se pudo obtener un ID de la segunda memoria compartida\n");
		exit(4);
	}

    // Adosar el proceso al espacio de memoria mediante un puntero
    memoria_comp1 = (struct datos *) shmat(IDmem1, (const void *)0,0);
    if (memoria_comp1 == NULL){
		printf("No se pudo asociar el puntero a la primera memoria compartida\n");
		exit(5);
	}
    memoria_comp2 = (struct datos *) shmat(IDmem2, (const void *)0,0);
    if (memoria_comp2 == NULL){
		printf("No se pudo asociar el puntero a la primera memoria compartida\n");
		exit(6);
	}

    // Creación de semaforos
    IDsem = semget(clave, 3, 0666 | IPC_CREAT);
    if (IDsem == -1){
        printf("No se puede crear el semáforo\n");
        exit(7);
    }

    // Inicialización de semaforos
    op.sem_flg = 0;                 // Nunca usamos flags para los semaforos

    argumento.val = 1; //Semaforo de sincronizacion inicializado en verde
    semctl (IDsem, SEM_SYNC, SETVAL, argumento);
    argumento.val = 0; //Semaforo de lectura inicializado en rojo
    semctl (IDsem, SEM_READ, SETVAL, argumento);
    argumento.val = 1; //Semaforo de escritura inicializado en verde
    semctl (IDsem, SEM_WRITE, SETVAL, argumento);

    // Verificar que el archivo exista
    fpdat = fopen("datos.dat","rb");
    if (fpdat == 0) {
        printf("No se puede abrir el archivo.\n");
        return 0;
    }

    gettimeofday(&tiempo, NULL);                // Obtengo el tiempo de UNIX inicial, al momento que se escribe el primer dato
    tiempo_init = tiempo.tv_usec;
    
    // Se lee el archivo binario
    fread(&(buffer.dato),sizeof(struct datos),1,fpdat);
    int memcomp_cnt=0;
    memoria_comp[0].id=-1;
    while(!feof(fpdat)){
            memoria_comp1[memcomp_cnt].dato = buffer.dato;                  // Copia de datos al buffer

            memoria_comp1[memcomp_cnt].id += 1;                             // Asigno ID al dato, que se incrementa por cada dato que se lee
        
            gettimeofday(&tiempo, NULL);
            memoria_comp1[memcomp_cnt].tiempo = tiempo.tv_usec - tiempo_init;      // Le resto el tiempo inicial al tiempo actual para obtener el timestamp

            fread(&(buffer.dato),sizeof(struct datos),1,fpdat);
            memcomp_cnt++;
    }
    fclose(fpdat);

    // Se libera la memoria compartida
    shmdt ((const void *) memoria_comp1);
    shmdt ((const void *) memoria_comp2);

	shmctl (IDmem1, IPC_RMID, (struct shmid_ds *)NULL);
    shmctl (IDmem2, IPC_RMID, (struct shmid_ds *)NULL);
    return 0;
}