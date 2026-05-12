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
    SHAREDSTATS *stats;
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
int produz(ProdutorArgs *args, char *line);
/**
 * 
 */
void consome(char *line, CONFIG *config, SHAREDSTATS *stats);
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