/**
 * @file R3_1_logAnalyzer.h
 * @brief Recebe os argumentos pela linha de comandos e analisa a log
 */
#ifndef R3_1_LOG_ANALYZER_H
#define R3_1_LOG_ANALYZER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "event_classifier.h"

/**
 * @struct config
 * @brief esta struct permite armazenar os parametros que vao ser analisados
 */
typedef struct config
{
    char *diretorio;
    int numProcessos;
    int numThreads;
    int modo;
    int verbose;
    char *outFiles;
} CONFIG;

/**
 * @brief Funcao que recebe os parametros da linha de comandos e preenche a struct
 * @param argc   Numero de argumentos passados na linha de comandos.
 * @param argv   Array de strings com os argumentos.
 * @param config Ponteiro para a struct CONFIG que sera preenchida.
 */
void parseArguments(int argc, char *argv[], CONFIG *config);

#endif