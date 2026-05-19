#include "R4_3_prodConsBoundedBuffer.h"

pthread_mutex_t mutex_prod = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cons = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_attacks = PTHREAD_MUTEX_INITIALIZER;
sem_t podeProduzir;
sem_t podeConsumir;

LogEntry buffer[MAX_BUFFER];

SHAREDSTATS globalStats = {0, 0, 0};
int erros_5xx_consecutivos = 0;
int falhas_auth_consecutivas = 0; // Variáveis para os ataques e tentativas brute force

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
void consome(LogEntry log_recebido, CONFIG *config, SHAREDSTATS *stats) {
    long consumeErrors = 0;
    long consumeWarnings = 0;

    pthread_mutex_lock(&mutex_attacks); // Protege a lógica dos ataques
    
    switch (config->modo) {
        case MODE_SECURITY: // Exemplo: Brute force
            if (log_recebido.type == FORMAT_SYSLOG && log_recebido.is_auth_failure) {
                consumeErrors++;
                falhas_auth_consecutivas++;
                if (falhas_auth_consecutivas >= 5) { 
                    printf("[ALERTA] Brute-Force detetado!\n");
                    falhas_auth_consecutivas = 0;
                }
            } else {
                falhas_auth_consecutivas = 0;
            }
            break;
            
        case MODE_PERFORMANCE: // Exemplo: 5xx consecutivos
            if (log_recebido.status_code >= 500) {
                consumeErrors++;
                erros_5xx_consecutivos++;
                if (erros_5xx_consecutivos >= 10) {
                    printf("[ALERTA] 10 Erros 5xx consecutivos!\n");
                    erros_5xx_consecutivos = 0;
                }
            } else if (log_recebido.status_code > 0 && log_recebido.status_code < 500) {
                erros_5xx_consecutivos = 0;
            }
            break;
            
    
    }
    pthread_mutex_unlock(&mutex_attacks);

    pthread_mutex_lock(&mutex_cons);
    stats->total_lines++;
    stats->errors += consumeErrors;
    stats->warnings += consumeWarnings;
    pthread_mutex_unlock(&mutex_cons);
}



void *produtor(void *arg) {
    ProdutorArgs *args = (ProdutorArgs*)arg;
    char line[4096];
    
    for (int i = 0; i < args->num_ficheiros_atribuidos; i++) {
        char *nome_ficheiro = args->ficheiros_atribuidos[i];
        LogFormat formato_atual = formatCase(nome_ficheiro);
        
        int fd = open(nome_ficheiro, O_RDONLY);
        if (fd == -1) continue;

        while (produz(fd, line) > 0) { 
            
            LogEntry log_atual;
            memset(&log_atual, 0, sizeof(LogEntry));
            log_atual.type = formato_atual;
            
            // 🚨 FALTAVA: O Produtor é que faz o Parse!
            switch (formato_atual) {
                case FORMAT_APACHE:
                    parse_apache_log(line, (ApacheLogEntry*)&log_atual); // Atenção: as tuas funções originais recebiam a struct ApacheLogEntry, JSONLogEntry, etc. Tens de confirmar se o teu cast funciona ou se precisas de alterar o teu parse para aceitar LogEntry
                    break;
                case FORMAT_JSON:
                    parse_json_log(line, (JSONLogEntry*)&log_atual);
                    break;
                case FORMAT_NGINX:
                    parse_nginx_error(line, (NGINX_ERROR*)&log_atual); //ver
                    break;
                case FORMAT_SYSLOG:
                    parse_syslog(line, (SyslogEntry*)&log_atual);
                    break;
                default:
            }

            sem_wait(&podeProduzir);
            pthread_mutex_lock(&mutex_prod);
            
            buffer[prodptr] = log_atual; // Mete a struct inteira
            prodptr = (prodptr + 1) % MAX_BUFFER;
            
            pthread_mutex_unlock(&mutex_prod);
            sem_post(&podeConsumir);
        }
        close(fd);
    }
    return NULL;
}
    
void *consumidor(void *arg){
    ConsumidorArgs *args = (ConsumidorArgs*)arg;
    
    while (TRUE) {
        LogEntry log_recebido; 
        
        sem_wait(&podeConsumir);
        pthread_mutex_lock(&mutex_cons);
        
        log_recebido = buffer[consptr]; 

        consptr = (consptr + 1) % MAX_BUFFER; // buffer circular
        
        pthread_mutex_unlock(&mutex_cons);
        sem_post(&podeProduzir);
        
    
        if (log_recebido.type== FORMAT_UNKNOWN && log_recebido.status_code == -1) {
            break; 
        }

        consome(log_recebido, args->config, args->stats); // Passamos a struct
    }
    return NULL;
}


void logWorkerProducerConsumer(CONFIG *config){
    int numProd = config->numProdutores;
    int numCons = config->numConsumidores;
    pthread_t tProd[numProd];
    pthread_t tCons[numCons];
    
    sem_init(&podeProduzir, 0, MAX_BUFFER);
    sem_init(&podeConsumir, 0, 0);
    
    ProdutorArgs argsProd[numProd];
    ConsumidorArgs argsCons[numCons];

    char ficheiros[100][512];
    int numFicheiros = listFiles(config->diretorio, ficheiros, 100);

    // atualiza os argumentos do produtor
    for(long i = 0; i < numProd; i++){ 
        argsProd[i].config = config;
        argsProd[i].id = i;
        argsProd[i].num_ficheiros_atribuidos = 0;
    }

    for (int i = 0; i < numFicheiros; i++) {
        int prod_id = i % numProd;
        int idx_no_cesto = argsProd[prod_id].num_ficheiros_atribuidos;
        strcpy(argsProd[prod_id].ficheiros_atribuidos[idx_no_cesto], ficheiros[i]);
        argsProd[prod_id].num_ficheiros_atribuidos++;
    }

    for(long i = 0; i < numProd; i++){
        pthread_create(&tProd[i], NULL, produtor, &argsProd[i]);
    }
    
    for(long i = 0; i < numCons; i++){
        argsCons[i].config = config;
        argsCons[i].id = i;
        argsCons[i].stats = &globalStats;
        pthread_create(&tCons[i], NULL, consumidor, &argsCons[i]);
    }

    for(long i = 0; i < numProd; i++){
        pthread_join(tProd[i], NULL);
    }
    
    // Poison pill
    for(int i = 0; i < numCons; i++) {
        LogEntry poison;
        memset(&poison, 0, sizeof(LogEntry));
        poison.type = FORMAT_UNKNOWN;
        poison.status_code = -1;

        sem_wait(&podeProduzir);
        pthread_mutex_lock(&mutex_prod);
        buffer[prodptr] = poison;
        prodptr = (prodptr + 1) % MAX_BUFFER;
        pthread_mutex_unlock(&mutex_prod);
        sem_post(&podeConsumir);
    }

    for(long i = 0; i < numCons; i++){
        pthread_join(tCons[i], NULL);
    }
    
    sem_destroy(&podeProduzir);
    sem_destroy(&podeConsumir);

    printf("Total de linhas: %ld\nErros: %ld\nAvisos: %ld\n", globalStats.total_lines, globalStats.errors, globalStats.warnings);
    printf("Tentativas de Brute force: %ld\nErros HTTP 5xx: %ld\n", falhas_auth_consecutivas, erros_5xx_consecutivos);
}