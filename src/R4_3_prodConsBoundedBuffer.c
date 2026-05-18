#include "R4_3_prodConsBoundedBuffer.h"

pthread_mutex_t mutex_prod = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cons = PTHREAD_MUTEX_INITIALIZER;
sem_t podeProduzir;
sem_t podeConsumir;

LogEntry buffer[MAX_BUFFER];

SHAREDSTATS stats = {0, 0, 0};

int prodptr = 0, consptr = 0;




int produz(int fd, char *line){
  int pos=0;
  char character;
  ssize_t n; // para guardar valores negativos para erros
  while((n = read(fd, &character, 1))>0){ //enquanto não chegamos ao fim do ficheiro
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



void *produtor(void *arg) {
    ProdutorArgs *args = (ProdutorArgs*)arg;
    char line[4096];
    
    for (int i = 0; i < args->num_ficheiros_atribuidos; i++) {
        char *nome_ficheiro = args->ficheiros_atribuidos[i];
        LogFormat formato_atual = formatCase(nome_ficheiro); // Detetive em ação!
        
        args->fd = open(nome_ficheiro, O_RDONLY);
        if (args->fd == -1) continue;

        while (produz(args->fd, line) > 0) { // produz() lê uma linha e retorna 1 se sucesso
            
            LogEntry log_atual;
            memset(&log_atual, 0, sizeof(LogEntry)); // Limpa o lixo de memória
            log_atual.format = formato_atual;
            
            // O produtor faz o parse e preenche a struct
            switch (formato_atual) {
                case FORMAT_APACHE:
                    parse_apache_log(line, (ApacheLogEntry*)&log_atual); // Adapta as tuas funções de parse para preencherem esta struct genérica
                    break;
                // ... (outros formatos)
            }

            // Insere no Buffer (Bloqueia se cheio)
            sem_wait(&podeProduzir);
            pthread_mutex_lock(&mutex_prod);
            
            buffer[prodptr] = log_atual; // Mete a struct no array
            prodptr = (prodptr + 1) % MAX_BUFFER;
            
            pthread_mutex_unlock(&mutex_prod);
            sem_post(&podeConsumir); // Avisa os consumidores que há comida
        }
        close(args->fd);
    }
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

       // 1. Inicializar as estruturas
for(long i = 0; i < numProd; i++){
    argsProd[i].config = config;
    argsProd[i].id = i;
    argsProd[i].stats = &stats;
    argsProd[i].num_ficheiros_atribuidos = 0; // Começam com o cesto vazio
}

// 2. Distribuir os ficheiros um a um (Round-Robin)
for (int i = 0; i < numFicheiros; i++) {
    int prod_id = i % numProd; // Vai rodando: 0, 1, 2, 0, 1, 2...
    int idx_no_cesto = argsProd[prod_id].num_ficheiros_atribuidos;
    
    strcpy(argsProd[prod_id].ficheiros_atribuidos[idx_no_cesto], ficheiros[i]);
    argsProd[prod_id].num_ficheiros_atribuidos++;
}

// 3. Agora sim, criar as threads!
for(long i = 0; i < numProd; i++){
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