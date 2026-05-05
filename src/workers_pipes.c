#include "workers_pipes.h"

// ---------------------------------------- FUNCAO PRINCIPAL ----------------------------------------

void logWorker_pipes(CONFIG *config){
    int nWorkers = config->numProcessos;
 
    char ficheiros[100][512];
    int numFicheiros = listFiles(config->diretorio, ficheiros, 100);
    if (numFicheiros <= 0) {
        write(STDOUT_FILENO, "Nenhum ficheiro .log encontrado.\n", 33);
        return;
    }
 
    // um pipe para cada filho (matriz onde [0] é leitura e [1] é escrita)
    int pipes[nWorkers][2];
    pid_t pids[nWorkers];
 
    // Divisão das tarefas: quantos arquivos cada processo filho vai processar
    int arquivos_por_filho = numFicheiros / nWorkers;
    int remain = numFicheiros % nWorkers; // arquivos que sobraram da divisão exata
 
    int start = 0; // marca de onde o filho atual começa a ler na lista de arquivos
 
    for (int i = 0; i < nWorkers; i++) {
 
        // Cria o pipe ANTES do fork, senão o filho não consegue herdar a comunicação!
        if (pipe(pipes[i]) == -1) {
            perror("ERRO NO PIPE");
            exit(EXIT_FAILURE);
        }
 
        // Se ainda tem 'sobras', dá um arquivo a mais pra esse filho
        int count = arquivos_por_filho + (i < remain ? 1 : 0);
        int end = start + count;
 
        // Cria o processo filho
        pids[i] = fork();
 
        if (pids[i] == -1) {
            perror("ERRO NO FORK");
            exit(EXIT_FAILURE);
        }
 
        if (pids[i] == 0) { // Entrou no código do FILHO
 
            close(pipes[i][0]); // O filho só escreve, então fecha a leitura do próprio pipe
 
            // fechar os outros pipes (os filhos herdam os pipes dos irmãos mais velhos que o pai já tinha aberto)
            for (int j = 0; j < i; j++) { 
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
 
            // Chama a função que faz o trabalho pesado e passa o lado de ESCRITA do pipe (pipes[i][1])
            filho_logic(pipes[i][1], i, config, ficheiros, start, end);
            // A função filho_logic já tem um exit() no final, então ele morre lá dentro.
        }
 
        // Código do PAI:
        close(pipes[i][1]); // O pai só lê, então fecha o lado de escrita desse filho
 
        start = end; // O próximo filho começa de onde esse parou
    }
 
    // Variaveis do pai para somar tudo o que os filhos mandarem
    long total_lines   = 0;
    long total_errors  = 0;
    long total_warnings = 0;
 
    int workers_active = nWorkers;
    
    // Vetor para marcar quais filhos já terminaram (1 = acabou, 0 = rodando)
    int done[nWorkers];
    memset(done, 0, sizeof(done)); // zera o vetor
 
    // O pai fica aqui nesse loop até todos os filhos morrerem
    while (workers_active > 0) {
        for (int i = 0; i < nWorkers; i++) {
            if (done[i]) continue; // Se esse filho já acabou, pula pro próximo
 
            /* * DICA:
             * "Mas esse readn não trava o pai se o filho demorar?"
             * Resposta: "Sim. A leitura é síncrona. O pai espera o filho 'i' mandar algo. 
             * Para evitar isso, teríamos que usar chamadas mais complexas, mas para já, basta."
             */
            Message msg;
            ssize_t n = readn(pipes[i][0], &msg, sizeof(msg));
 
            if (n <= 0) {
                // Se leu 0 ou deu erro, o pipe fechou (filho acabou ou deu pau)
                done[i] = 1;
                workers_active--;
                close(pipes[i][0]);
                continue;
            }
 
            // Verifica o tipo da mensagem recebida pelo pipe e soma nas estatísticas gerais
            if (msg.type == MSG_TYPE_NORMAL && msg.size == sizeof(NormalMsg)) {
                NormalMsg norm_msg;
                readn(pipes[i][0], &norm_msg, sizeof(norm_msg));
 
                total_lines += norm_msg.total_lines;
                total_errors += norm_msg.errors;
                total_warnings += norm_msg.warnings;
 
                char buf[256];
                int len = snprintf(buf, sizeof(buf),
                    "[Worker %d] PID:%d | Linhas:%ld | Erros:%ld | Avisos:%ld | Top IP:%s\n",
                    i, norm_msg.pid, norm_msg.total_lines, norm_msg.errors, norm_msg.warnings, norm_msg.top_ip);
                write(STDOUT_FILENO, buf, len);
 
            } else if (msg.type == MSG_TYPE_VERBOSE && msg.size == sizeof(VerboseMsg)) {
                VerboseMsg verb_msg;
                readn(pipes[i][0], &verb_msg, sizeof(verb_msg));
 
                char buf[512];
                int  len = snprintf(buf, sizeof(buf),
                    "[VERBOSE] PID:%d | %s | %s | IP:%s | %s\n",
                    verb_msg.pid, verb_msg.timestamp, verb_msg.type, verb_msg.ip, verb_msg.msg);
                write(STDOUT_FILENO, buf, len);
 
            } else if (msg.type == MSG_TYPE_DONE) {
                // Filho avisou explicitamente que terminou
                done[i] = 1;
                workers_active--;
                close(pipes[i][0]);
            }
        }
    }
 
    // Padrão: fazer o waitpid para limpar os processos zumbis que ficaram
    for (int i = 0; i < nWorkers; i++) {
        waitpid(pids[i], NULL, 0);
    }
 
    // Print do relatório final na tela
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

// Funções clássicas readn e writen para lidar com pipes e sockets.
// Garantem que tudo será lido/escrito, mesmo se o SO interromper o processo no meio (EINTR).
ssize_t readn(int fd, void *ptr, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *buf = ptr;
 
    while (nleft > 0) {
        nread = read(fd, buf, nleft);
        if (nread < 0) {
            if (errno == EINTR) {
                continue; // se foi interrupção de sinal, tenta de novo
            }
            return -1;
        } else if (nread == 0) {
            break; // chegou no final (EOF)
        }
        nleft -= nread;
        buf += nread;
    }
    return (ssize_t)(n - nleft);
}
 
ssize_t writen(int fd, const void *ptr, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
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
        buf += nwritten;
    }
    return (ssize_t)n;
}

void send_msg(int fd_write, int type, const void *payload, int size){ // o payload não sabe o que vai receber, logo, colocamos o void *
    Message msg;
    msg.type = type;
    msg.size = size;

    // Manda o cabeçalho primeiro (a struct Message)
    if(writen(fd_write, &msg, sizeof(msg)) < 0){
        perror("FILHO: erro ao enviar o header pelo pipe");
        exit(EXIT_FAILURE);
    }

    // Depois manda os dados em si (o payload)
    if(size > 0){
        if(writen(fd_write, payload, size) < 0){
            perror("FILHO: erro ao enviar o payload pelo pipe");
            exit(EXIT_FAILURE);
        }
    }
}

// Função onde o filho realmente trabalha.
// Recebe a lista de arquivos e lê os arquivos que estão entre 'start' e 'end'.
void filho_logic(int fd_write, int id, CONFIG *config, char ficheiros[][512], int start, int end){ // o start e end servem para dividir as tarefas!
    NormalMsg norm_msg;
    VerboseMsg verb_msg;

    memset(&norm_msg, 0, sizeof(norm_msg));
    norm_msg.pid = getpid();
    strcpy(norm_msg.top_ip, "N/A"); // N/A : not avaliable!

    // controle dos IPs para estatística
    char ip_list[256][64];
    long ip_count[256];
    int ip_total = 0;
    memset(ip_list,  0, sizeof(ip_list));
    memset(ip_count, 0, sizeof(ip_count));

    ApacheLogEntry log_apache;
    JSONLogEntry log_json;
    SyslogEntry log_syslog;
    NginxErrorEntry log_nginx;

    for(int i = start; i < end; i++){

        int file = open(ficheiros[i], O_RDONLY);

        if(file < 0){
            perror("FILHO: erro ao abrir o ficheiro");
            continue; // se der erro em um arquivo, pula pro próximo
        }

        char buffer[2048]; // armazena a linha que estamos construindo
        int  pos = 0;
        char c;
        ssize_t n; 

        // Lendo caractere por caractere. Não é o jeito mais rápido, mas é o mais fácil de programar e entender.
        while ((n = read(file, &c, 1)) > 0) {
            // Vai guardando no buffer até achar o \n ou encher
            if (pos < (int)sizeof(buffer) - 1) {
                buffer[pos++] = c;
            }
 
            if (c == '\n') {
                buffer[pos] = '\0'; // fecha a string
                pos = 0; // reseta para a próxima linha
                norm_msg.total_lines++;
 
                // Verifica qual formato estamos usando e processa a linha montada
                switch (config->modo) {
                    case 1:
                        if (parse_apache_log(buffer, &log_apache) == 0) {
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
                                norm_msg.errors++;
                                if (config->verbose) { // modo tagarela ligado
                                    memset(&verb_msg, 0, sizeof(verb_msg));
                                    verb_msg.pid = getpid();
                                    strftime(verb_msg.timestamp, sizeof(verb_msg.timestamp), "%Y-%m-%d %H:%M:%S", &log_apache.timestamp);
                                    strcpy(verb_msg.type, "ERROR");
                                    snprintf(verb_msg.msg, sizeof(verb_msg.msg), "HTTP %d em %s", log_apache.status_code, log_apache.url);
                                    strncpy(verb_msg.ip, log_apache.ip, 63);
                                    send_msg(fd_write, MSG_TYPE_VERBOSE, &verb_msg, sizeof(verb_msg));
                                }
                            } else if (log_apache.status_code >= 400) {
                                norm_msg.warnings++;
                            }
                        }
                        break;
 
                    case 2:
                        if (parse_json_log(buffer, &log_json) == 0) {
                            if (log_json.level == LOG_ERROR || log_json.level == LOG_CRITICAL) {
                                norm_msg.errors++;
                                if (config->verbose) {
                                    memset(&verb_msg, 0, sizeof(verb_msg));
                                    verb_msg.pid = getpid();
                                    strftime(verb_msg.timestamp, sizeof(verb_msg.timestamp), "%Y-%m-%d %H:%M:%S", &log_json.timestamp);
                                    strcpy(verb_msg.type, log_json.level == LOG_CRITICAL ? "CRITICAL" : "ERROR");
                                    strncpy(verb_msg.msg, log_json.message, 255);
                                    strncpy(verb_msg.ip,  log_json.ip, 63);
                                    send_msg(fd_write, MSG_TYPE_VERBOSE, &verb_msg, sizeof(verb_msg));
                                }
                            } else if (log_json.level == LOG_WARN) {
                                norm_msg.warnings++;
                            }
                            norm_msg.total_lines++;
                        }
                        break;
 
                    case 3:
                        if (parse_syslog(buffer, &log_syslog) == 0) {
                            if (log_syslog.is_auth_failure) {
                                norm_msg.errors++;
                                if (config->verbose) {
                                    memset(&verb_msg, 0, sizeof(verb_msg));
                                    verb_msg.pid = getpid();
                                    strftime(verb_msg.timestamp, sizeof(verb_msg.timestamp), "%Y-%m-%d %H:%M:%S", &log_syslog.timestamp);
                                    strcpy(verb_msg.type, "CRITICAL");
                                    strncpy(verb_msg.msg, log_syslog.message, 255);
                                    strncpy(verb_msg.ip,  log_syslog.hostname, 63);
                                    send_msg(fd_write, MSG_TYPE_VERBOSE, &verb_msg, sizeof(verb_msg));
                                }
                            } else if (log_syslog.is_sudo_attempt ||
                                       log_syslog.is_firewall_block) {
                                norm_msg.warnings++;
                            }
                            norm_msg.total_lines++;
                        }
                        break;
 
                    case 4:
                        if (parse_nginx_error(buffer, &log_nginx) == 0) {
                            if (log_nginx.level >= NGINX_ERROR) {
                                norm_msg.errors++;
                                if (config->verbose) {
                                    memset(&verb_msg, 0, sizeof(verb_msg));
                                    verb_msg.pid = getpid();
                                    strftime(verb_msg.timestamp, sizeof(verb_msg.timestamp), "%Y-%m-%d %H:%M:%S", &log_nginx.timestamp);
                                    strcpy(verb_msg.type, "ERROR");
                                    strncpy(verb_msg.msg, log_nginx.message, 255);
                                    strncpy(verb_msg.ip,  log_nginx.client_ip, 63);
                                    send_msg(fd_write, MSG_TYPE_VERBOSE, &verb_msg, sizeof(verb_msg));
                                }
                            } else if (log_nginx.level == NGINX_WARN) {
                                norm_msg.warnings++;
                            }
                            norm_msg.total_lines++;
                        }
                        break;
 
                    default:
                        break;
                }
            }
        }
 
        close(file);
    }
 
    // Procura qual foi o IP que mais apareceu
    long max_count = 0;
    for (int k = 0; k < ip_total; k++) {
        if (ip_count[k] > max_count) { // maior IP
            max_count = ip_count[k];
            strncpy(norm_msg.top_ip, ip_list[k], 63);
        }
    }
 
    // Envia o resumo geral que esse filho processou
    send_msg(fd_write, MSG_TYPE_NORMAL, &norm_msg, sizeof(norm_msg));
 
    // Envia sinal que terminou para liberar o pai do "while"
    send_msg(fd_write, MSG_TYPE_DONE, NULL, 0); 
 
    close(fd_write);
    exit(EXIT_SUCCESS); // Importante: garante que o filho morre aqui e não continua rodando o código do pai
}