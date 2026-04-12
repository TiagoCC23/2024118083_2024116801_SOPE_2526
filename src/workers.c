#include "workers.h"

void logWorker(CONFIG *config) {
    int nWorkers = config->numProcessos;
    __pid_t pids[nWorkers];
    int fdsDuplo[nWorkers][2]; 

    char ficheiros[100][512];
    int numFicheiros = listFiles(config->diretorio, ficheiros, 100);
    // recrutmento de workers
    for(int i = 0; i < nWorkers; i++) {
        
        // criaçao do pipe
        if (pipe(fdsDuplo[i]) == -1) {
            perror("Erro ao criar o pipe\n");
            exit(EXIT_FAILURE);
        }

        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("Erro ao criar o fork\n");
            exit(EXIT_FAILURE);
        } 
        
        if (pids[i] == 0) {
            close(fdsDuplo[i][1]); // porta de escrita fechada

            for (int j = 0; j < i; j++) {
                close(fdsDuplo[j][0]);
                close(fdsDuplo[j][1]);
            }

            // O Filho salta para a sua função de trabalho!
            workersLogic(fdsDuplo[i][0], i, config);
  
        } 
        else {
            close(fdsDuplo[i][0]); // Pai fecha a ponta de leitura
        }
    }
    
    int fdLog = open(config->diretorio, O_RDONLY);
    if(fdLog == -1){
        perror("Erro ao abrir a log\n");
        exit(EXIT_FAILURE);
    } 

    char c;
    char linhaBuffer[2048];
    int pos = 0;
    int workerAtual = 0;
    
    while (read(fdLog, &c, 1) > 0) {
        linhaBuffer[pos] = c;
        pos++;
        
        if (c == '\n') {
            write(fdsDuplo[workerAtual][1], linhaBuffer, pos);
            pos = 0;
            
            workerAtual++;
            if (workerAtual >= nWorkers) { 
                workerAtual = 0;
            }
        }
    }
    close(fdLog);

    // o pai fecha o pipe porque o ficheiro já acabou
    for (int i = 0; i < nWorkers; i++) {
        close(fdsDuplo[i][1]);
    }

    // ciclo para evitar zombies
    for (int i = 0; i < nWorkers; i++) {
        wait(NULL);
    }
           
    printf("\n[Director] Todos os workers acabaram com sucesso\n");
}


// Função dedicada ao trabalho do Filho
void workersLogic(int fd_leitura, int id, CONFIG *config) {
    char buffer[1024];
    ssize_t bytesLidos;
    
    ApacheLogEntry log_apache;
    JSONLogEntry log_json;
    SyslogEntry log_syslog;
    NginxErrorEntry log_nginx;
    PipeMessage message;

    while ((bytesLidos = read(fd_leitura, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesLidos] = '\0';
        printf("Worker %d recebeu: %s", id, buffer);

        switch (config->modo) {    
            case 1:
                if(parse_apache_log(buffer, &log_apache) == 0) {
                    //TODO
                    
                }  
                break;
            case 2:
                if(parse_json_log(buffer, &log_json) == 0) {
                    //TODO
                }   
                break;
            case 3:
                if(parse_syslog(buffer, &log_syslog) == 0) {
                    //TODO
                message.total_lines++;
                if(log_syslog.is_auth_failure){
                    message.errors++;
                } else if(log_syslog.is_sudo_attempt){
                    message.warnings++;
                } else if (log_syslog.is_firewall_block)
                {
                    message.warnings++; // ver se faz mais sentido incrementar o erro ou aviso
                }
                
                }  
                break;
            case 4:
                if(parse_nginx_error(buffer, &log_nginx) == 0) {
                    //TODO
                    message.total_lines++;
                    if(log_nginx.level >= NGINX_ERROR){
                    message.errors++;
                    } else if(log_nginx.level == NGINX_WARN){
                        message.warnings++;
                    }
                }   
                break;                
            default:
                break;
        }
    }
    
    close(fd_leitura); // porta de leitura fechada
    exit(EXIT_SUCCESS);   
}
int listFiles(const char *diretorio, char ficheiros[][512], int maxFicheiros){
    DIR *dir = opendir(diretorio);
    if(dir==NULL){
        perror("Erro ao abrir o diretorio ");
        return -1;
    }
    struct dirent *input;
    int count=0;
    while ((input= readdir(dir)) !=NULL)
    {
        if(strstr(input->d_name, ".log")!=NULL){
            snprintf(ficheiros[count], 512, "%s/%s", diretorio, input->d_name);
            count++;
        }
    }
    closedir(dir);
    return count;
}