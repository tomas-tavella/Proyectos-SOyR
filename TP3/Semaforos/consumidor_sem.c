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
#define SEM_BUF1 1
#define SEM_BUF2 2

// Definir las operaciones para los semaforos
#define BLOCK(OP,sem) {\
    OP.sem_num = sem; \
    OP.sem_op = -1; \
    semop(IDsem, &OP, 1);\
}
#define UNBLOCK(OP,sem) {\
    OP.sem_num = sem; \
    OP.sem_op = +1; \
    semop(IDsem, &OP, 1);\
}

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
    char dato[17];
};

int main(){
    key_t clave1, clave2, clavesem;
    int IDbuf1, IDbuf2, IDsem;
    struct datos *buf1 = NULL;
    struct datos *buf2 = NULL;
    union semun argumento;
    struct sembuf op;
    char dato_val[17];

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
    IDsem = semget(clavesem, 3, 0666);
    if (IDsem == -1){
        printf("No se puede crear el semáforo\n");
        exit(4);
    }

    // Inicialización de semaforos
    op.sem_flg = 0;

    // Verificar que el archivo exista
    fpcsv = fopen("datos.csv","w");
    if (fpcsv == 0) {
        printf("No se puede crear el archivo.\n");
        return 0;
    }

    int buf_cnt=0;
    int pos_eof=-1;                          // Para almacenar la posicion del eof dentro del buffer
    //int buf_select=0;
    while (1){
        // Comienzo seccion critica (leer buffer 1)
        BLOCK(op,SEM_BUF1);
        while(/*buf_select == 0 && */buf_cnt < CANTIDAD && buf1[buf_cnt].id != -1){
            fprintf(fpcsv,"%d,%ld us,%s\n",buf1[buf_cnt].id,buf1[buf_cnt].tiempo,buf1[buf_cnt].dato);
            printf("%d,%ld us,%s\n",buf1[buf_cnt].id,buf1[buf_cnt].tiempo,buf1[buf_cnt].dato);
            buf_cnt++;

            if (buf1[buf_cnt].id == -1){
                pos_eof = buf_cnt;
                break;
            }
        }
        buf_cnt = 0;
        UNBLOCK(op,SEM_BUF1);
        // Finalizo seccion critica (leer buffer 1)

        UNBLOCK(op,SEM_SYNC);       // Desbloqueo el semaforo de sincronizacion una vez que leo todos los datos de buf1

        // Comienzo seccion critica (leer buffer 2)
        BLOCK(op,SEM_BUF2);
        while(/*buf_select == 1 && */buf_cnt < CANTIDAD && buf2[buf_cnt].id != -1){
            fprintf(fpcsv,"%d,%ld us,%s\n",buf2[buf_cnt].id,buf2[buf_cnt].tiempo,buf2[buf_cnt].dato);
            printf("%d,%ld us,%s\n",buf2[buf_cnt].id,buf2[buf_cnt].tiempo,buf2[buf_cnt].dato);
            buf_cnt++;

            if (buf2[buf_cnt].id == -1){
                pos_eof = buf_cnt;
                break;
            }
        }
        buf_cnt = 0;
        //sleep(1);             // El SEM_SYNC anda como deberia probando con sleep en el consumidor
        UNBLOCK(op,SEM_BUF2);
        // Finalizo seccion critica (leer buffer 2)
        
        UNBLOCK(op,SEM_SYNC);       // Desbloqueo el semaforo de sincronizacion una vez que leo todos los datos de buf2

        if (pos_eof != -1/*buf1[buf_cnt-1].id == -1 || buf2[buf_cnt-1].id == -1*/) break;

        //buf_cnt=0;
        //buf_select = !(buf_select);

    }

    fclose(fpcsv);

    shmdt ((const void *) buf1);
    shmdt ((const void *) buf2);

    shmctl (IDbuf1, IPC_RMID, (struct shmid_ds *)NULL);
    shmctl (IDbuf2, IPC_RMID, (struct shmid_ds *)NULL);

    return 0;
}