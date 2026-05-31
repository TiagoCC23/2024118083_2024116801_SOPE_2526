/**
 * 
 */
#ifndef R4_3_PRODCONSBOUNDEDBUFFER_H
#define R4_3_PRODCONSBOUNDEDBUFFER_H
#include "R3_1_logAnalyzer.h"
#include "R4_1_threads.h"

#include<semaphore.h>
#define TRUE 1
#define MAX_BUFFER 10

/**
 * 
 */
typedef struct {
    CONFIG *config;
    int id;
    char ficheiros_atribuidos[100][512]; // O "cesto" de ficheiros deste produtor
    int num_ficheiros_atribuidos;        // Quantos ficheiros estão no cesto
} ProdutorArgs;
/**
 * 
 */
typedef struct {
    CONFIG *config;
    int id;
    SHAREDSTATS *stats; /**< Apontador para as stats */
} ConsumidorArgs;




/**
 * 
 */
int produz(int fd, char *line);
/**
 * 
 */
long consome(LogEntry log_recebido, CONFIG *config, SHAREDSTATS *stats);
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