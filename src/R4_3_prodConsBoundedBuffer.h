/**
 * 
 */
#ifndef R4_3_PRODCONSBOUNDEDBUFFER_H
#define R4_3_PRODCONSBOUNDEDBUFFER_H
#include "R4_1_threads.h"
#include<semaphore.h>
#define TRUE 1


/**
 * 
 */
typedef struct {
    char ficheiro[512];
    CONFIG *config;
    int id;
    int fd;
} ProdutorArgs;
/**
 * 
 */
typedef struct {
    CONFIG *config;
    int id;
} ConsumidorArgs;
/**
 * 
 */
typedef struct {
    char linha[4096];  // a linha de log
    int  ocupado;      // 0 = vazio, 1 = tem dados
} LogEntry;

/**
 * 
 */
int produz(ProdutorArgs *args, char *line);
/**
 * 
 */
void consome(char *line, CONFIG *config);

/**
 * 
 */
void *produtor(void *arg);
/**
 * 
 */
void *consumidor(void *arg);

/**
 * 
 */
void logWorkerProducerConsumer(CONFIG *config);


#endif