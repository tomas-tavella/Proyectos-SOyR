#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>

// Definir puntero al archivo a utilizar
FILE *fpcsv;

// Definir los parametros de ftok
#define NUMERO1 30 
#define NUMERO2 31 
#define NUMEROSEM 32   
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
    key_t clave1, clave2, clavesem;
    int IDbuf1, IDbuf2, IDsem;
    struct datos *buf1 = NULL;
    struct datos *buf2 = NULL;
    union semun argumento;
    struct sembuf op;

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
    clavesem = ftok(PATH,NUMEROSEM);
    if (clavesem == (key_t) -1){
		printf("No se pudo obtener la clave del semaforo\n");
		exit(3);
	}

    // Llamar al sistema para obtener la memoria compartida
    IDbuf1 = shmget(clave1, CANTIDAD*sizeof(struct datos), 0666);
    if(IDbuf1 == -1){
		printf("No se pudo obtener un ID de la primera memoria compartida\n");
		exit(3);
	}
    IDbuf2 = shmget(clave2, CANTIDAD*sizeof(struct datos), 0666);
    if(IDbuf2 == -1){
		printf("No se pudo obtener un ID de la segunda memoria compartida\n");
		exit(4);
	}

    // Adosar el proceso al espacio de memoria mediante un puntero
    buf1 = (struct datos *) shmat(IDbuf1, (const void *)0,0);
    if (buf1 == NULL){
		printf("No se pudo asociar el puntero a la primera memoria compartida\n");
		exit(5);
	}
    buf2 = (struct datos *) shmat(IDbuf2, (const void *)0,0);
    if (buf2 == NULL){
		printf("No se pudo asociar el puntero a la primera memoria compartida\n");
		exit(6);
	}


    // Creación de semaforos
    IDsem = semget(clavesem, 3, 0666 | IPC_CREAT);
    if (IDsem == -1){
        printf("No se puede crear el semáforo\n");
        exit(4);
    }

    // Inicialización de semaforos
    op.sem_flg = 0;                 // Nunca usamos flags para los semaforos

    // Verificar que el archivo exista
    fpcsv = fopen("datos.csv","w");
    if (fpcsv == 0) {
        printf("No se puede crear el archivo.\n");
        return 0;
    }

    int buf_cnt=0;
    int buf_select=0;
    while (buf1[buf_cnt].id != NULL || buf2[buf_cnt].id != NULL){               // Si el ID no indica EOF, sigo leyendo
        op.sem_num = SEM_READ;
        BLOQUEAR(op);
        semop(IDsem, &op, 3);
        // Copiar datos a csv
        op.sem_num = SEM_WRITE;
        BLOQUEAR(op);
        semop(IDsem, &op, 3);
    }
    /*
    // Espero a que el productor desbloquee el semaforo de sincronizacion para poder leer
    op.sem_num = SEM_SYNC;
    BLOQUEAR(op);
    semop(IDsem, &op, 3);

    int memcomp_cnt = 0;
    int auxBuffer=0; //Variable auxiliar para ver de que buffer leer
    while (memcomp_cnt < 2*CANTIDAD){
        if(auxBuffer==0){
            fprintf(fpcsv,"%d,%ld,%f\n",buf1[memcomp_cnt].id,buf1[memcomp_cnt].tiempo,buf1[memcomp_cnt].dato);
        }
        else{
            fprintf(fpcsv,"%d,%ld,%f\n",buf2[memcomp_cnt].id,buf2[memcomp_cnt].tiempo,buf2[memcomp_cnt].dato);
        }
        memcomp_cnt++;
        if(memcomp_cnt==CANTIDAD){
                memcomp_cnt=0;
                auxBuffer = !(auxBuffer);
        }
    }
    // Desbloqueo el semaforo de sincronizacion
    op.sem_num = SEM_SYNC;
    DESBLOQUEAR(op);
    semop(IDsem, &op, 3);
    */
    fclose(fpcsv);

    return 0;
}