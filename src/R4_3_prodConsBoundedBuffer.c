#include "R4_3_prodConsBoundedBuffer.h"

pthread_mutex_t mutex_prod = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cons = PTHREAD_MUTEX_INITIALIZER;
sem_t podeProduzir;
sem_t podeConsumir;
int prodptr = 0, constptr = 0;
//provisório
#define N 10
SHAREDSTATS stats = {0, 0, 0};

char buffer[N][4096];


int produz(ProdutorArgs *args, char *line){
  int pos=0;
  char character;
  ssize_t n; // para guardar valores negativos para erros
  while((n = read(args->fd, &character, 1))>0){ //enquanto não chegamos ao fim do ficheiro
        if(pos < 4095){
            line[pos++]= character;
        }
        if(character == '\n'){
            line[pos]='\0';
            return 1; // leitura feita com sucesso
        }
  }
  if(pos > 0){ // ultima linha sem \n para possível conteudo acumulado no buffer
    line[pos]= '\0';
    return 1;
  }
  return 0; // EOF
}
void consome(char *line, CONFIG *config, SHAREDSTATS *stats){
    ApacheLogEntry log_apache;
    JSONLogEntry log_json;
    SyslogEntry log_syslog;
    NginxErrorEntry log_nginx;
    long consumeErrors = 0;
    long consumeWarnings = 0;
     switch (config->modo)
        {
        case 1:
            if(parse_apache_log(line, &log_apache) == 0) {
                if(log_apache.status_code >= 500){
                        consumeErrors++;
                    } else if(log_apache.status_code >=400){
                        consumeWarnings++;
                    }
            }
            break;
        case 2:
            if(parse_json_log(line, &log_json) == 0) {
                    if(log_json.level == LOG_ERROR ||log_json.level == LOG_CRITICAL){
                        consumeErrors++;
                    } else if (log_json.level == LOG_WARN){
                        consumeWarnings++;
                    }
            }
            break;
        case 3:
            if(parse_syslog(line, &log_syslog) == 0) {
                if(log_syslog.is_auth_failure){
                    consumeErrors++;
                } else if(log_syslog.is_sudo_attempt || log_syslog.is_firewall_block){
                   consumeWarnings++;
                } 
            }
            break;
        case 4:
           if(parse_nginx_error(line, &log_nginx) == 0) {
            if(log_nginx.level >= NGINX_ERROR){
                    consumeErrors++;
                    } else if(log_nginx.level == NGINX_WARN){
                        consumeWarnings++;
                    }
           }
            break;
        default:
            break;
        }
    pthread_mutex_lock(&mutex_cons);
    stats->total_lines++;
    stats->errors+=consumeErrors;
    stats->warnings+=consumeWarnings;
    pthread_mutex_unlock(&mutex_cons);
}



void *produtor(void *arg){
    ProdutorArgs *args = (ProdutorArgs*)arg;
    
    if (args->ficheiro[0] == '\0') {
        return NULL; 
    }

    // abre o ficheiro antes do loop
    args->fd = open(args->ficheiro, O_RDONLY);
    if(args->fd == -1){
        perror("Produtor: erro ao abrir ficheiro");
        return NULL;
    }

    char line[4096];
    while (TRUE) {
        // lê uma linha do ficheiro
        if(produz(args->fd, line) == 0){
         break; // EOF
        }
        sem_wait(&podeProduzir);
        pthread_mutex_lock(&mutex_prod);
        strcpy(buffer[prodptr], line); // guarda a linha no buffer
        prodptr = (prodptr + 1) % N; // buffer circular
        pthread_mutex_unlock(&mutex_prod);
        sem_post(&podeConsumir);
    }

    close(args->fd);
    return NULL;
}
    
void *consumidor(void *arg){
    ConsumidorArgs *args = (ConsumidorArgs*)arg;
    
    char linha[4096];
    while (TRUE) {
        sem_wait(&podeConsumir);
        pthread_mutex_lock(&mutex_cons);
        strcpy(linha, buffer[constptr]); // copia a linha do buffer
        buffer[constptr][0] = '\0';      // limpa a posição
        constptr = (constptr + 1) % N; // buffer circular
        pthread_mutex_unlock(&mutex_cons);
        sem_post(&podeProduzir);
        
        consome(linha, args->config, &stats); // processa a linha
    }
    return NULL;
}


void logWorkerProducerConsumer(CONFIG *config){
    int numProd = config->numProdutores;
    int numCons = config->numConsumidores;
    pthread_t tProd[numProd];
    pthread_t tCons[numCons];
    pthread_t printProdCons;
    sem_init(&podeProduzir, 0, N);
    sem_init(&podeConsumir, 0, 0);
    ProdutorArgs argsProd[numProd];
    ConsumidorArgs argsCons[numCons];

    char ficheiros[100][512];
    int numFicheiros = listFiles(config->diretorio, ficheiros, 100);

        for(long i=0; i<numProd; i++){
        argsCons[i].config = config;
        argsCons[i].id = i;
        argsCons[i].stats = &stats;
        if(i<numFicheiros){
        strcpy(argsProd[i].ficheiro, ficheiros[i]);
        } else{
            argsProd[i].ficheiro[0] = '\0';
        }
        pthread_create(&tProd[i], NULL, produtor, &argsProd[i]);
        }
        for(long i=0; i<numCons; i++){
        argsCons[i].config = config;
        argsCons[i].id = i;
        pthread_create(&tCons[i], NULL, consumidor, &argsCons[i]);
        }
    //pthread_create(&printProdCons, NULL, fotoProdCons,NULL); Chamo a função dashboard feita pela RaySantos
    //pthread_join(printProdCons, NULL);
        for(long i=0; i<numProd; i++){
        pthread_join(tProd[i],NULL);
        }
        for(long i=0; i<numCons; i++){
        pthread_join(tCons[i],NULL);
        }
    sem_destroy(&podeProduzir);
    sem_destroy(&podeConsumir);

    printf("Total de linhas: %ld\nErros: %ld\nAvisos: %ld\n",stats.total_lines, stats.errors, stats.warnings);


}