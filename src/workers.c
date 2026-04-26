#include "workers.h"

void logWorker(CONFIG *config) {
    int nWorkers = config->numProcessos;
    __pid_t pids[nWorkers];
    int fdsDuplo[nWorkers][2];  // array de pipes para cada worker

    // descobre todos os ficheiros .log
    char ficheiros[100][512];
    int numFicheiros = listFiles(config->diretorio, ficheiros, 100);
    // recrutmento de workers onde são criados n processos
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

            // O Filho salta para a sua função de trabalho
            workersLogic(fdsDuplo[i][0], i, config, numFicheiros);
  
        } 
        else {
            close(fdsDuplo[i][0]); // pai fecha a ponta de leitura
        }
    }
    for (int i = 0; i < numFicheiros; i++) {
    int worker = i % nWorkers;                     // percorre os ficheiros e garante que cada worker recebe o mesmo número de ficheiros
    write(fdsDuplo[worker][1], ficheiros[i], 512); // divide pelos workers para depois escrever em cada um
}
    int fdLog = open(config->diretorio, O_RDONLY); // desaparece no 3.3
    
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
        
        if (c == '\n') { // chegou ao fim da linha
            write(fdsDuplo[workerAtual][1], linhaBuffer, pos); // manda a linha para o pipe
            pos = 0;
            workerAtual++; // avança o worker e se chegou ao fim, volta ao primeiro para continuar a escrita
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
void workersLogic(int fd_leitura, int id, CONFIG *config, int numFIles) {
    char buffer[1024];
    ssize_t bytesLidos;
    
    ApacheLogEntry log_apache;
    JSONLogEntry log_json;
    SyslogEntry log_syslog;
    NginxErrorEntry log_nginx;
    PipeMessage message;
    message.total_lines=0;
    message.errors=0;
    message.warnings=0; // inicia a 0 para evitar lixo de memória
    for(int i=0; i<numFIles; i++){
    char path[512];
    read(fd_leitura, path, 512); //recebe o caminho para abrir-se os ficheiros
    int fdFile = open(path, O_RDONLY);
    if(fdFile == -1){
        perror("Erro ao abrir o ficheiro ");
        exit(EXIT_FAILURE);
    }

    // le linha a linha para depois aplicar os parsers
    while ((bytesLidos = read(fdFile, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesLidos] = '\0';
        printf("Worker %d recebeu: %s", id, buffer);

        switch (config->modo) {    
            case 1:
                if(parse_apache_log(buffer, &log_apache) == 0) {
                    message.total_lines++;
                    if(log_apache.status_code >= 500){
                        message.errors++;
                    } else if(log_apache.status_code >=400){
                        message.warnings++;
                    }
                    }
                break;
            case 2:
                if(parse_json_log(buffer, &log_json) == 0) {
                    message.total_lines++;
                    if(log_json.level == LOG_ERROR ||log_json.level == LOG_CRITICAL){
                        message.errors++;
                    } else if (log_json.level == LOG_WARN)
                    {
                        message.warnings++;
                    }
                    
                }   
                break;
            case 3:
                if(parse_syslog(buffer, &log_syslog) == 0) {

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
    char nameResult[64];
    snprintf(nameResult, sizeof(nameResult), "results_%d.txt", (int) getpid()); // vai printar no ficheiro
    int fdResult = open(nameResult, O_WRONLY | O_CREAT | O_APPEND, 0644); // escreve so no fim sem sobrescrita e o dono escreve e lê (4+2) e o grupo e os outros podem ler
    if(fdResult == -1){                                                   // no ficheiro
        perror("Erro ao criar o ficheiro ");
        exit(EXIT_FAILURE);
    }
    close(fdFile); // porta de leitura fechada
    char result[256];
    int size = snprintf(result, sizeof(result), "PID:%d;FICHEIRO:%s;LINHAS:%ld;ERRORS:%ld;WARNINGS:%ld\n",(int)getpid(),path, message.total_lines, message.errors, message.warnings);
    write(fdResult, result,size );
    close(fdResult);
}
    exit(EXIT_SUCCESS);  
}


int listFiles(const char *diretorio, char ficheiros[][512], int maxFicheiros){
    DIR *dir = opendir(diretorio); // abre o ficheiro
    if(dir==NULL){
        perror("Erro ao abrir o diretorio ");
        return -1;
    }
    struct dirent *input; // guarda o input atual
    int count=0;          // contador de logs
    while ((input= readdir(dir)) !=NULL)    // enquanto nao ficarmos sem inputs no ficheiro
    {
        if(strstr(input->d_name, ".log")!=NULL){   // verifica se o ficheiro acaba em .log para incrementar o contador
            snprintf(ficheiros[count], 512, "%s/%s", diretorio, input->d_name);
            count++;
        }
    }
    closedir(dir);
    return count;
}