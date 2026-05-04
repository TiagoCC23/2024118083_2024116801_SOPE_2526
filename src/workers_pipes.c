#include "workers_pipes.h"

// ---------------------------------------- FUNCAO PRINCIPAL ----------------------------------------

void logWorker_pipes(CONFIG *config){
    int nWorkers = config->numProcessos;
 
    char ficheiros[100][512];
    int  numFicheiros = listFiles(config->diretorio, ficheiros, 100);
    if (numFicheiros <= 0) {
        write(STDOUT_FILENO, "Nenhum ficheiro .log encontrado.\n", 44);
        return;
    }
 
    // um pipe para cada filho
    int pipes[nWorkers][2];
    pid_t pids[nWorkers];
 
    int base = numFicheiros / nWorkers;
    int remain = numFicheiros % nWorkers;
 
    int start = 0;
 
    for (int i = 0; i < nWorkers; i++) {
 
        if (pipe(pipes[i]) == -1) {
            perror("ERRO NO PIPE");
            exit(EXIT_FAILURE);
        }
 
        int count = base + (i < remain ? 1 : 0);
        int end   = start + count;
 
        pids[i] = fork();
 
        if (pids[i] == -1) {
            perror("ERRO NO FORK");
            exit(EXIT_FAILURE);
        }
 
        if (pids[i] == 0) { // filho
 
            close(pipes[i][0]); // fecha a leitura
 
            for (int j = 0; j < i; j++) { // fechar os outros pipes
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
 
            filho_logic(pipes[i][1], i, config, ficheiros, start, end);
        }
 
        close(pipes[i][1]); // fecha o lado de leitura desse filho
 
        start = end;
    }
 
    // pai
    long total_lines   = 0;
    long total_errors  = 0;
    long total_warnings = 0;
 
    int workers_active = nWorkers;
    int done[nWorkers];
    memset(done, 0, sizeof(done));
 
    while (workers_active > 0) {
        for (int i = 0; i < nWorkers; i++) {
            if (done[i]) continue;
 
            MsgHeader msg;
            ssize_t n = readn(pipes[i][0], &msg, sizeof(hdr));
 
            if (n <= 0) {
                done[i] = 1;
                workers_active--;
                close(pipes[i][0]);
                continue;
            }
 
            if (msg.type == MSG_TYPE_NORMAL && msg.size == sizeof(NormalMsg)) {
                NormalMsg nmsg;
                readn(pipes[i][0], &nmsg, sizeof(nmsg));
 
                total_lines    += nmsg.total_lines;
                total_errors   += nmsg.errors;
                total_warnings += nmsg.warnings;
 
                char buf[256];
                int  len = snprintf(buf, sizeof(buf),
                    "[Worker %d] PID:%d | Linhas:%ld | Erros:%ld | Avisos:%ld | Top IP:%s\n",
                    i, nmsg.pid, nmsg.total_lines,
                    nmsg.errors, nmsg.warnings, nmsg.top_ip);
                write(STDOUT_FILENO, buf, len);
 
            } else if (msg.type == MSG_TYPE_VERBOSE && msg.size == sizeof(VerboseMsg)) {
                VerboseMsg vmsg;
                readn(pipes[i][0], &vmsg, sizeof(vmsg));
 
                char buf[512];
                int  len = snprintf(buf, sizeof(buf),
                    "[VERBOSE] PID:%d | %s | %s | IP:%s | %s\n",
                    vmsg.pid, vmsg.timestamp, vmsg.type,
                    vmsg.ip, vmsg.msg);
                write(STDOUT_FILENO, buf, len);
 
            } else if (msg.type == MSG_TYPE_DONE) {
                done[i] = 1;
                workers_active--;
                close(pipes[i][0]);
            }
        }
    }
 
    for (int i = 0; i < nWorkers; i++) {
        waitpid(pids[i], NULL, 0);
    }
 
    char report[512];
    int  rlen = snprintf(report, sizeof(report),
        "\n>>>>>> RELATÓRIO FINAL <<<<<<\n"
        "Ficheiros processados: %d\n"
        "Total de linhas: %ld\n"
        "Total de erros: %ld\n"
        "Total de avisos: %ld\n"
        "\n",
        numFicheiros, total_lines, total_errors, total_warnings);
    write(STDOUT_FILENO, report, rlen);
}

// ---------------------------------------- FUNCOES ADICIONAIS ----------------------------------------

ssize_t readn(int fd, void *ptr, size_t n) {
    size_t  nleft = n;
    ssize_t nread;
    char   *buf = ptr;
 
    while (nleft > 0) {
        nread = read(fd, buf, nleft);
        if (nread < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        } else if (nread == 0) {
            break;
        }
        nleft -= nread;
        buf   += nread;
    }
    return (ssize_t)(n - nleft);
}
 
ssize_t writen(int fd, const void *ptr, size_t n) {
    size_t      nleft = n;
    ssize_t     nwritten;
    const char *buf = ptr;
 
    while (nleft > 0) {
        nwritten = write(fd, buf, nleft);
        if (nwritten < 0) {
            if (errno == EINTR) { 
                continue;
            }
            return -1; 
        }
        nleft -= nwritten;
        buf   += nwritten;
    }
    return (ssize_t)n;
}

void send_msg(int fd_write, int type, const void *payload, int size){ // o payload não sabe o que vai receber, logo, colocamos o void *
    Message msg;120
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

        while ((n = read(fdFile, &c, 1)) > 0) {
            if (pos < (int)sizeof(linebuf) - 1) {
                linebuf[pos++] = c;
            }
 
            if (c == '\n') {
                linebuf[pos] = '\0';
                pos = 0;
                nmsg.total_lines++;
 
                switch (config->modo) {
                    case 1:
                        if (parse_apache_log(linebuf, &log_apache) == 0) {
                            int found = 0;
                            for (int k = 0; k < ip_total; k++) {
                                if (strcmp(ip_list[k], log_apache.ip) == 0) {
                                    ip_count[k]++;
                                    found = 1;
                                    break;
                                }
                            }
                            if (!found && ip_total < 256) {
                                strncpy(ip_list[ip_total], log_apache.ip, 63);
                                ip_count[ip_total] = 1;
                                ip_total++;
                            }
 
                            if (log_apache.status_code >= 500) {
                                nmsg.errors++;
                                if (config->verbose) {
                                    memset(&vmsg, 0, sizeof(vmsg));
                                    vmsg.pid = getpid();
                                    strncpy(vmsg.timestamp, log_apache.timestamp, 31);
                                    strcpy(vmsg.type, "ERROR");
                                    snprintf(vmsg.msg, sizeof(vmsg.msg),
                                             "HTTP %d em %s", log_apache.status_code, log_apache.url);
                                    strncpy(vmsg.ip, log_apache.ip, 63);
                                    send_msg(fd_write, MSG_TYPE_VERBOSE, &vmsg, sizeof(vmsg));
                                }
                            } else if (log_apache.status_code >= 400) {
                                nmsg.warnings++;
                            }
                        }
                        break;
 
                    case 2:
                        if (parse_json_log(linebuf, &log_json) == 0) {
                            if (log_json.level == LOG_ERROR ||
                                log_json.level == LOG_CRITICAL) {
                                nmsg.errors++;
                                if (config->verbose) {
                                    memset(&vmsg, 0, sizeof(vmsg));
                                    vmsg.pid = getpid();
                                    strncpy(vmsg.timestamp, log_json.timestamp, 31);
                                    strcpy(vmsg.type,
                                           log_json.level == LOG_CRITICAL ? "CRITICAL" : "ERROR");
                                    strncpy(vmsg.msg, log_json.message, 255);
                                    strncpy(vmsg.ip,  log_json.ip, 63);
                                    send_msg(fd_write, MSG_TYPE_VERBOSE, &vmsg, sizeof(vmsg));
                                }
                            } else if (log_json.level == LOG_WARN) {
                                nmsg.warnings++;
                            }
                            nmsg.total_lines++;
                        }
                        break;
 
                    case 3:
                        if (parse_syslog(linebuf, &log_syslog) == 0) {
                            if (log_syslog.is_auth_failure) {
                                nmsg.errors++;
                                if (config->verbose) {
                                    memset(&vmsg, 0, sizeof(vmsg));
                                    vmsg.pid = getpid();
                                    strncpy(vmsg.timestamp, log_syslog.timestamp, 31);
                                    strcpy(vmsg.type, "CRITICAL");
                                    strncpy(vmsg.msg, log_syslog.message, 255);
                                    strncpy(vmsg.ip,  log_syslog.ip, 63);
                                    send_msg(fd_write, MSG_TYPE_VERBOSE, &vmsg, sizeof(vmsg));
                                }
                            } else if (log_syslog.is_sudo_attempt ||
                                       log_syslog.is_firewall_block) {
                                nmsg.warnings++;
                            }
                            nmsg.total_lines++;
                        }
                        break;
 
                    case 4:
                        if (parse_nginx_error(linebuf, &log_nginx) == 0) {
                            if (log_nginx.level >= NGINX_ERROR) {
                                nmsg.errors++;
                                if (config->verbose) {
                                    memset(&vmsg, 0, sizeof(vmsg));
                                    vmsg.pid = getpid();
                                    strncpy(vmsg.timestamp, log_nginx.timestamp, 31);
                                    strcpy(vmsg.type, "ERROR");
                                    strncpy(vmsg.msg, log_nginx.message, 255);
                                    strncpy(vmsg.ip,  log_nginx.client_ip, 63);
                                    send_msg(fd_write, MSG_TYPE_VERBOSE, &vmsg, sizeof(vmsg));
                                }
                            } else if (log_nginx.level == NGINX_WARN) {
                                nmsg.warnings++;
                            }
                            nmsg.total_lines++;
                        }
                        break;
 
                    default:
                        break;
                }
            }
        }
 
        close(fdFile);
    }
 
    long max_count = 0;
    for (int k = 0; k < ip_total; k++) {
        if (ip_count[k] > max_count) { // maior IP
            max_count = ip_count[k];
            strncpy(nmsg.top_ip, ip_list[k], 63);
        }
    }
 
    send_msg(fd_write, MSG_TYPE_NORMAL, &nmsg, sizeof(nmsg)); // resumo do que aconteceu
 
    send_msg(fd_write, MSG_TYPE_DONE, NULL, 0); // mensagem pro pai
 
    close(fd_write);
    exit(EXIT_SUCCESS);
}

