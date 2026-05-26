/**
 * 
 */
#ifndef R4_3_PRODCONSBOUNDEDBUFFER_H
#define R4_3_PRODCONSBOUNDEDBUFFER_H
#include "R4_1_threads.h"
#include "R3_4_R4_2_dashboard.h"
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


typedef enum { 
    LOG_APACHE, 
    LOG_JSON, 
    LOG_SYSLOG, 
    LOG_NGINX 
} LogType;

typedef struct {
    LogType type;
    int status_code;          // Útil para erros 5xx (Apache/Nginx)
    int level;                // Para JSON e Nginx
    int is_auth_failure;      // Para Syslog (útil para brute-force)
    int is_sudo_attempt;      // Para Syslog
    int is_firewall_block;    // Para Syslog
    char ip[64];              // Essencial para rastrear os IPs do brute-force
} LogEntry;

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