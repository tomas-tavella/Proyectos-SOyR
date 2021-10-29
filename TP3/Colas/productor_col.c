#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
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

// Estructura de datos a escribir en el buffer
struct datos{
    int id;
    suseconds_t tiempo;               // susesconds_t esta incluido en <sys/types.h> y devuelve el tiempo en micro segundos
    char dato[30];
}buffer;


int main(int argc, char *argv[]){
    key_t clave;
    int IDmem;
    struct datos *memoria_comp = NULL;
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
        strcpy(memoria_comp[memcomp_cnt].dato , buffer.dato);         // Copia de datos al buffer

        memoria_comp[memcomp_cnt].id += 1;                             // Asigno ID al dato, que se incrementa por cada dato que se lee
        
        gettimeofday(&tiempo, NULL);
        memoria_comp[memcomp_cnt].tiempo = tiempo.tv_usec - tiempo_init;      // Le resto el tiempo inicial al tiempo actual para obtener el timestamp

        fread(&(buffer.dato),sizeof(struct datos),1,fpdat);
        memcomp_cnt++;
    }
    fclose(fpdat);

    // Se libera la memoria compartida
    shmdt ((const void *) memoria_comp);

	shmctl (IDmem, IPC_RMID, (struct shmid_ds *)NULL);
    return 0;
}