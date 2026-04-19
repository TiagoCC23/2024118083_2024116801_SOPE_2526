#ifndef THREADS_H
#define THREADS_H
#include <pthread.h>
#include <sys/stat.h>
#include "log_parser.h"
#include "logAnalyzer.h"
#include "event_classifier.h"

typedef struct {
    int id;                // id do thread
    char *buffer_completo; // ponteiro para o texto do ficheiro
    long offset_start;    // Onde esta thread começa a ler
    long offset_end;       // Onde esta thread para
    CONFIG *config;        // Configurações (modo, etc.)
} THREADDATA;
void* thread_worker(void* arg);
void logWorkerThreads(CONFIG *config);









#endif