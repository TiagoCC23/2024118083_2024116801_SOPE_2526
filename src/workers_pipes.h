#include "log_parser.h"
#include "logAnalyzer.h"
#include "workers.h"

#define MSG_TYPE_NORMAL 1
#define MSG_TYPE_VERBOSE 2
#define MSG_TYPE_DONE 3

// ------------------ ESTRUTURAS ------------------
typedef struct {
    pid_t  pid;
    long   total_lines;
    long   errors;
    long   warnings;
    char   top_ip[64];
} NormalMsg;

typedef struct {
    pid_t  pid;
    char   timestamp[32];
    char   type[16]; // se eh: INFO, WARN, ERROR, CRITICAL
    char   msg[256];
    char   ip[64];
} VerboseMsg;

typedef struct {
    int  type; // vem do msg_type_*
    int  size; // tamanho do payload a seguir
} Message;

// ------------------ FUNCAO PRINCIPAL ------------------
void logWorker_pipes(CONFIG *config);

// ------------------ FUNCOES ADICIONAIS ------------------
ssize_t readn(int fd, void *ptr, size_t n);
ssize_t writen(int fd, const void *ptr, size_t n);
void send_msg(int fd_write, int type, const void *payload, int size);
void filho_logic(int fd_write, int id, CONFIG *config, char ficheiros[][512], int start, int end);