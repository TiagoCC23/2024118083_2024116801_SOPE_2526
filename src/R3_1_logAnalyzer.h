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
    int numProdutores;
    int numConsumidores;
} CONFIG;

/**
 * @struct LogFormat
 * @brief útil para o switch case onde vamoss escolher os formatos
 */
typedef enum {
    FORMAT_APACHE,
    FORMAT_JSON,
    FORMAT_SYSLOG,
    FORMAT_NGINX,
    FORMAT_UNKNOWN
} LogFormat;

// A caixa universal que viaja no buffer (e nos pipes/sockets)
typedef struct {
    LogFormat format;          // Para o consumidor saber de onde veio
    
    // Campos comuns
    char timestamp[64];
    char ip[64];               // Essencial para rastrear os IPs do brute-force
    
    // Apache & Nginx
    int status_code;           // Útil para erros 5xx (Apache/Nginx)
    
    // JSON & Nginx
    int level;                 // Nível de erro (INFO, WARN, ERROR, etc.)
    
    // Syslog
    int is_auth_failure;
    int is_sudo_attempt;
    int is_firewall_block;    
    
} LogEntry;

/**
 * @brief Funcao que recebe os parametros da linha de comandos e preenche a struct
 * @param argc   Numero de argumentos passados na linha de comandos.
 * @param argv   Array de strings com os argumentos.
 * @param config Ponteiro para a struct CONFIG que sera preenchida.
 */
void parseArguments(int argc, char *argv[], CONFIG *config);

/**
 * 
 */
LogFormat formatCase(const char *filePatch);

#endif