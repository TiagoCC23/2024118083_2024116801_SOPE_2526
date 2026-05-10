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
int produz();
/**
 * 
 */
void consome(int item);

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