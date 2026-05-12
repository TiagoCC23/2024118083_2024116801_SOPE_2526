/**
 * @file R4_1_threads.h
 * @brief Implementação do processamento de logs utilizando múltiplas threads partilhando a mesma memória (RAM).
 */
#ifndef R4_1_THREADS_H
#define R4_1_THREADS_H
#include <pthread.h>
#include <sys/stat.h>
#include "log_parser.h"
#include "R3_1_logAnalyzer.h"
#include "event_classifier.h"

/**
 * @struct THREADDATA
 * @brief Estrutura que serve de armazena o que cada thread tem
 */
typedef struct {
    int id;                   /**< id do thread */ 
    char *buffer_completo;    /**< ponteiro para o texto do ficheiro */ 
    long offset_start;        /**< Onde esta thread começa a ler */ 
    long offset_end;          /**< Onde esta thread para
    */ CONFIG *config;        /**< Configurações (modo, etc.) */ 
} THREADDATA;

/**
 * 
 */
typedef struct {
    long total_lines;
    long errors;
    long warnings;
} SHAREDSTATS;
/**
 * @brief Função executada por cada thread. Processa a sua fatia de memória, linha a linha, e atualiza as estatísticas globais.
 * @param arg Ponteiro genérico que será convertido para a struct THREADDATA contendo os limites de leitura da thread.
 * @return Retorna NULL quando a thread termina a sua fatia de trabalho.
 */
void* threadWorker(void* arg);

/**
 * @brief Função principal (Diretor) do Requisito 4.1. Carrega o ficheiro para a RAM, calcula os blocos de cada thread e coordena a execução.
 * @param config Ponteiro para a struct CONFIG com os parâmetros recebidos pela linha de comandos (ficheiro, nº de threads, modo).
 */
void logWorkerThreads(CONFIG *config);


#endif