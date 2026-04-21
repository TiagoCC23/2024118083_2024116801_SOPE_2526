#include "workers_pipes.h"

// ---------------------------------------- FUNCAO PRINCIPAL ----------------------------------------

void logWorker_pipes(CONFIG *config){

}

// ---------------------------------------- FUNCOES ADICIONAIS ----------------------------------------

ssize_t readn(int fd, void *ptr, size_t n) {
    size_t  nleft = n;
    ssize_t nread;
    char   *buf = ptr;
 
    while (nleft > 0) {
        nread = read(fd, buf, nleft);
        if (nread < 0) {
            if (errno == EINTR)   /* interrompido por sinal, tentar de novo */
                continue;
            return -1;            /* erro real */
        } else if (nread == 0) {
            break;                /* EOF */
        }
        nleft -= nread;
        buf   += nread;
    }
    return (ssize_t)(n - nleft);  /* bytes efectivamente lidos */
}
 
ssize_t writen(int fd, const void *ptr, size_t n) {
    size_t      nleft = n;
    ssize_t     nwritten;
    const char *buf = ptr;
 
    while (nleft > 0) {
        nwritten = write(fd, buf, nleft);
        if (nwritten < 0) {
            if (errno == EINTR)   /* interrompido por sinal, tentar de novo */
                continue;
            return -1;            /* erro real */
        }
        nleft -= nwritten;
        buf   += nwritten;
    }
    return (ssize_t)n;
}

void send_msg(int fd_write, int type, const void *payload, int size){ // o payload não sabe o que vai receber, logo, colocamos o void *
    Message msg;
    msg.type = type;
    msg.size = size;

    if(writen(fd_write, &msg, sizeof(msg)) < 0){
        perror("FILHO: erro ao enviar o header pelo pipe");
        exit(EXIT_FAILURE);
    }

    if(size > 0){
        if(writen(fd_write, payload, size) < 0){
            perror("FILHO: erro ao enviar o payload pelo pipe");
            exit(EXIT_FAILURE);
        }
    }
}

void filho_logic(int fd_write, int id, CONFIG *config, char ficheiros[][512], int start, int end){ // o start e end servem para dividir as tarefas!
    NormalMsg norm_msg;
    VerboseMsg verb_msg;

    memset(&nmsg, 0, sizeof(nmsg));
    nmsg.pid = getpid();
    strcpy(nmsg.top_ip, "N/A"); // N/A : not avaliable!

    char ip_list[256][64];
    long ip_count[256];
    int ip_total = 0;
    memset(ip_list,  0, sizeof(ip_list));
    memset(ip_count, 0, sizeof(ip_count));

    ApacheLogEntry log_apache;
    JSONLogEntry   log_json;
    SyslogEntry    log_syslog;
    NginxErrorEntry log_nginx;

    for(int i = start; i < end; i++){

        int file = open(ficheiros[i], O_RDONLY);

        if(file < 0){
            perror("FILHO: erro ao abrir o ficheiro");
            continue;
        }

        char buffer[2048]; // para cada linha
        int  pos = 0;
        char c;
        ssize_t n; // serve como um int de ficheiros!

        
    }
}

