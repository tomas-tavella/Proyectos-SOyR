#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/time.h>                   // Para obtener el tiempo de UNIX

// Definir puntero al archivo a utilizar
FILE *fpdat;

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
    struct timeval tiempo, tiempo_init;
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
    IDbuf1 = shmget(clave1, CANTIDAD*sizeof(struct datos), 0666 | IPC_CREAT);
    if(IDbuf1 == -1){
		printf("No se pudo obtener un ID de la primera memoria compartida\n");
		exit(4);
	}
    IDbuf2 = shmget(clave2, CANTIDAD*sizeof(struct datos), 0666 | IPC_CREAT);
    if(IDbuf2 == -1){
		printf("No se pudo obtener un ID de la segunda memoria compartida\n");
		exit(5);
	}

    // Adosar el proceso al espacio de memoria mediante un puntero
    buf1 = (struct datos *) shmat(IDbuf1, (const void *)0,0);
    if (buf1 == NULL){
		printf("No se pudo asociar el puntero a la primera memoria compartida\n");
		exit(6);
	}
    buf2 = (struct datos *) shmat(IDbuf2, (const void *)0,0);
    if (buf2 == NULL){
		printf("No se pudo asociar el puntero a la primera memoria compartida\n");
		exit(7);
	}

    // Creación de semaforos
    IDsem = semget(clavesem, 3, 0666 | IPC_CREAT);
    if (IDsem == -1){
        printf("No se puede crear el semáforo\n");
        exit(8);
    }

    // Inicialización de semaforos
    op.sem_flg = 0;
    argumento.val = 0; //Semaforo de sincronizacion en rojo
    semctl (IDsem, SEM_SYNC, SETVAL, argumento);
    argumento.val = 1; //Semaforo del buffer 1 en verde
    semctl (IDsem, SEM_BUF1, SETVAL, argumento);
    argumento.val = 1; //Semaforo del buffer 2 en verde
    semctl (IDsem, SEM_BUF2, SETVAL, argumento);

    // Verificar que el archivo exista
    fpdat = fopen("datos.dat","rb");
    if (fpdat == 0) {
        printf("No se puede abrir el archivo.\n");
        return 0;
    }

    gettimeofday(&tiempo, NULL);                // Obtengo el tiempo de UNIX inicial, al momento que se escribe el primer dato
    tiempo_init = tiempo;
    
    // Se lee el archivo binario
    
    int buf_cnt=0;
    int id=0;
    int eof_flag=0;                   // Si es 1, termino en buf1, si es 2 en buf2
    
    while(1){

        // Comienzo una seccion critica (escribir buffer 1)
        // Tambien esta bloqueado el sem. de sincronizacion del final del loop anterior
        BLOCK(op,SEM_BUF1);
        fgets(dato_val,17,fpdat);
        while(buf_cnt<CANTIDAD){
            
            strcpy(buf1[buf_cnt].dato,dato_val);                        // Copio los datos a la memoria compartida

            buf1[buf_cnt].id = id;                                      // Asigno ID al dato, que se incrementa por cada dato que se lee
        
            gettimeofday(&tiempo, NULL);
            buf1[buf_cnt].tiempo = 1000000*(tiempo.tv_sec - tiempo_init.tv_sec) + (tiempo.tv_usec - tiempo_init.tv_usec);       // Le resto el tiempo inicial al tiempo actual para obtener el timestamp

            fgets(dato_val,17,fpdat);
            
            // Chequeo de EOF
            if (feof(fpdat)){           // Si se termina...
                eof_flag = 1;
                break;                  // Entonces salgo del loop
            }else{
                eof_flag = 0;
            }

            buf_cnt++; id++;
        }
        if (eof_flag == 1) break;       // Salgo del loop principal, conservando la posicion en buf1 donde se encontro EOF

        buf_cnt = 0;                    // Reseteo el contador
        UNBLOCK(op,SEM_BUF1);
        // Finalizo una seccion critica (escrbir buffer 1)

        // Comienzo una seccion critica (escribir buffer 2)
        BLOCK(op,SEM_BUF2);

        BLOCK(op,SEM_SYNC);         // Bloqueo el semaforo de sincronizacion, que es desbloqueado por el consumidor una vez que lee los datos de buf1
        fgets(dato_val,17,fpdat);
        while(buf_cnt<CANTIDAD){
            
            strcpy(buf2[buf_cnt].dato,dato_val);                        // Copio los datos a la memoria compartida

            buf2[buf_cnt].id = id;                                      // Asigno ID al dato, que se incrementa por cada dato que se lee
        
            gettimeofday(&tiempo, NULL);
            buf2[buf_cnt].tiempo = 1000000*(tiempo.tv_sec - tiempo_init.tv_sec) + (tiempo.tv_usec - tiempo_init.tv_usec);       // Le resto el tiempo inicial al tiempo actual para obtener el timestamp

            fgets(dato_val,17,fpdat);

            // Chequeo de EOF
            if (feof(fpdat)){           // Si se termina...
                eof_flag = 2;
                break;                  // Entonces salgo del loop
            }else{
                eof_flag = 0;
            }

            buf_cnt++; id++;
        }
        if (eof_flag == 2) break;       // Salgo del loop principal, conservando la posicion en buf2 donde se encontro EOF

        buf_cnt = 0;
        //sleep(1);             // El SEM_SYNC anda como deberia probando con sleep en el productor
        UNBLOCK(op,SEM_BUF2);
        // Finalizo una seccion critica (escribir buffer 2)

        BLOCK(op,SEM_SYNC);         // Bloqueo el semaforo de sincronizacion, que es desbloqueado por el consumidor una vez que lee los datos de buf1

    }
    // Se pone un ID NULL despues de llegar al ultimo elemento, para avisar al consumidor
    // que se llego al EOF
    switch (eof_flag){
        case 1 :
            buf1[buf_cnt].id = -1;
        case 2 :
            buf2[buf_cnt].id = -1;
    }

    fclose(fpdat);

    // Se libera la memoria compartida
    shmdt ((const void *) buf1);
    shmdt ((const void *) buf2);

	shmctl (IDbuf1, IPC_RMID, (struct shmid_ds *)NULL);
    shmctl (IDbuf2, IPC_RMID, (struct shmid_ds *)NULL);

    semctl (IDsem, SEM_SYNC, IPC_RMID);         // Remuevo los semaforos
    
    return 0;
}