/**
 * @file R3_2_workers.h
 * @brief Cria N processos filho, cada um responsável por processar um subconjunto dos
 * ficheiros de log encontrados no diretório.
 * Este módulo contém as structs necessárias e funções para a comunicação entre o pai e filho,
 * onde o pai distribui o trabalho e agrega os resultados enviados pelos filhos (workers).
 */
#ifndef R3_2_WORKERS_H
#define R3_2_WORKERS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>

#include "R3_1_logAnalyzer.h"
#include "event_classifier.h"
#include "log_parser.h"

/**
 * @struct PipeMessage
 * @brief Estrutura com os dados para o relatório final
 * 
 */
typedef struct {
    pid_t pid;                          /**< ID do processo filho */
    long total_lines;                   /**< Numero de linhas */
    long errors;                        /**< Numero de erros */
    long warnings;                      /**< Numero de avisos */
    char top_ip[16];                    /**< IP com o maior número de ocorrencias */
    int is_event;                       /**<  0 = resultado final; 1 = evento em tempo real */
    char timestamp[32];                 /**< Data e hora do evento. */
    char type[16];                      /**< Severidade: INFO, WARN, ERROR, CRITICAL. */
    char message[256];                  /**< menssagem a ser escrita */ 
    char ip[16];                        /**< Endereço IP associado ao evento. */
} PipeMessage;

/**
 * @brief Função principal do pai — descobre os ficheiros .log, cria N processos filho,
 * distribui os ficheiros entre eles e aguarda a sua terminação
 * @param config Configurações do utilizador (diretório, número de processos, modo, verbose)
 */
void logWorker(CONFIG *config);
/**
 * @brief Função executada por cada processo filho — recebe os caminhos dos ficheiros
 * pelo pipe, lê linha a linha, faz o parse e extrai métricas, e escreve os resultados
 * num ficheiro results_<pid>.txt
 * @param fd_leitura Descritor do pipe de leitura (recebe caminhos do pai)
 * @param id Identificador do worker (0 a N-1)
 * @param config Configurações do utilizador
 * @param numFIles Número de ficheiros que este worker vai processar
 */
void workersLogic(int fd_leitura, int id, CONFIG *config, int numFIles);
/**
 * @brief Descobre todos os ficheiros .log dentro de um diretório
 * @param diretorio Caminho da pasta a pesquisar
 * @param ficheiros Array de strings onde os caminhos completos são guardados
 * @param maxFicheiros Número máximo de ficheiros a guardar no array
 * @return Número de ficheiros .log encontrados, ou -1 em caso de erro
 */
int listFiles(const char *diretorio, char ficheiros[][512], int maxFicheiros);

#endif