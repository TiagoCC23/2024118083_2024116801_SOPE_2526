#include "logAnalyzer.h"

void parseArguments(int argc, char *argv[], CONFIG *config){
    if (argc<4){
        printf("Formato inválido!!\n");
        fprintf(stderr,"Tente algo como: ./logAnalyzer /var/log/apache2 4 security --verbose --output=report.json",argv[0]);
        exit(EXIT_FAILURE);
    }
    config->diretorio = argv[1];
    config->numProcessos = atoi(argv[2]);
    if(config->numProcessos <=0){
        fprintf(stderr,"Tem de haver pelo menos um processo");
        exit(EXIT_FAILURE);
    }
    
    // com base no event_classifier e com o que o utilizador escreveu, os else ifs servem para comparar o input dado com algo diferente que e recebido
    if (strcmp(argv[3], "security") == 0) {
        config->modo = MODE_SECURITY;
    } else if (strcmp(argv[3], "performance") == 0) {
        config->modo = MODE_PERFORMANCE;
    } else if (strcmp(argv[3], "traffic") == 0) {
        config->modo = MODE_TRAFFIC;
    } else if (strcmp(argv[3], "full") == 0) {
        config->modo = MODE_FULL;
    } else {
        fprintf(stderr, "Erro: Modo '%s' invalido.\n", argv[3]);
        exit(EXIT_FAILURE);
    }
    config->verbose = 0;
    config-> outFiles = NULL;


    // ciclo para procurar pelo verbose ou pelo output
    for (int i = 4; i < argc; i++)
    {
        if(strcmp(argv[i], "--verbose") == 0){
            config->verbose=1;
        } else if(strncmp(argv[i], "--output=", 9) == 0){
            config->outFiles = argv[i]+9;
        }
    }
    
}
void logWorker(CONFIG *config) {
    int nWorkers = config->numProcessos;
    __pid_t pids[nWorkers];
    int fdsDuplo[nWorkers][2]; 

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

            char buffer[1024];
            ssize_t bytesLidos;
            
            while ((bytesLidos = read(fdsDuplo[i][0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytesLidos] = '\0';
                 printf("Worker %d recebeu: %s", i, buffer);
                ApacheLogEntry log_apache;
                JSONLogEntry log_json;
                SyslogEntry log_syslog;
                NginxErrorEntry log_nginx;
                switch (config->modo)
                {    
                case 1:
                       if(parse_apache_log(buffer, &log_apache)==0){
                        //TODO
                        }  
                break;

                case 2:
                       if(parse_json_log(buffer, &log_json)==0){
                     //TODO
                    }   
                break;
                
                case 3:
                       if(parse_syslog(buffer, &log_syslog)==0){
                        //TODO
                        }  
                break;

                case 4:
                       if(parse_nginx_error(buffer, &log_nginx)==0){
                     //TODO
                    }   
                break;                
                
                default:
                    break;
                }
                {

                }
               
            }
            
            close(fdsDuplo[i][0]); // porta de leitura fechada
            exit(EXIT_SUCCESS);    
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