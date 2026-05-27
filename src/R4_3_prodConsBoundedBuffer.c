#include "R4_3_prodConsBoundedBuffer.h"
#include "R3_4_R4_2_dashboard.h"

pthread_mutex_t mutex_prod = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cons = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_attacks = PTHREAD_MUTEX_INITIALIZER;
sem_t podeProduzir;
sem_t podeConsumir;

LogEntry buffer[MAX_BUFFER];

SHAREDSTATS globalStats = {0, 0, 0};
int erros_5xx_consecutivos = 0;
int falhas_auth_consecutivas = 0; // Variáveis para os ataques e tentativas brute force
int trafego_consecutivo = 0;
int prodptr = 0, consptr = 0;


// Variáveis para o relatório final
int bruteForcesDetetatos = 0;
int erros5xxConsecutivosDetetados = 0;
int trafefoconsecutivosdetetados = 0;

int produz(int fd, char *line){
    int pos=0;
    char character;
    ssize_t n; // para guardar valores negativos para erros
    
    while((n = read(fd, &character, 1)) > 0){ //enquanto não chegamos ao fim do ficheiro
        if(pos < 4095){
            line[pos++] = character;
        }
        if(character == '\n'){
            line[pos] = '\0';
            return 1; // leitura feita com sucesso
        }
    }
    if(pos > 0){ // ultima linha sem \n para possível conteudo acumulado no buffer
        line[pos] = '\0';
        return 1;
    }
    return 0; // EOF
}

long consome(LogEntry log_recebido, CONFIG *config, SHAREDSTATS *stats) {
long consumeErrors = 0;
    long consumeWarnings = 0;

    pthread_mutex_lock(&mutex_attacks); // Protege a lógica dos ataques
    
    switch (config->modo) {
        case MODE_SECURITY: 
            if (log_recebido.type == FORMAT_SYSLOG && log_recebido.is_auth_failure) {
                consumeErrors++;
                falhas_auth_consecutivas++;
                if (falhas_auth_consecutivas >= 5) { 
                    bruteForcesDetetatos++;
                    falhas_auth_consecutivas = 0;
                }
            } else {
                falhas_auth_consecutivas = 0;
            }
            break;
            
        case MODE_PERFORMANCE: 
            if (log_recebido.status_code >= 500) {
                consumeErrors++;
                erros_5xx_consecutivos++;
                if (erros_5xx_consecutivos >= 10) {
                    erros5xxConsecutivosDetetados++;
                    erros_5xx_consecutivos = 0;
                }
            } else if (log_recebido.status_code > 0 && log_recebido.status_code < 500) {
                erros_5xx_consecutivos = 0;
            }
            break;
        
        case MODE_TRAFFIC:
            if(log_recebido.type == FORMAT_APACHE && log_recebido.status_code == 200){ 
                trafego_consecutivo++;
                if(trafego_consecutivo >= 50){ 
                    trafefoconsecutivosdetetados++;
                    trafego_consecutivo = 0;
               }
            } else {
                trafego_consecutivo = 0; 
            }
            break;
            
        case MODE_FULL:
            // Testa Brute Force
            if (log_recebido.type == FORMAT_SYSLOG && log_recebido.is_auth_failure) {
                consumeErrors++;
                falhas_auth_consecutivas++;
                if (falhas_auth_consecutivas >= 5) { 
                    bruteForcesDetetatos++;
                    falhas_auth_consecutivas = 0;
                }
            } else { falhas_auth_consecutivas = 0; }

            // Testa Performance 
            if (log_recebido.status_code >= 500) {
                consumeErrors++;
                erros_5xx_consecutivos++;
                if (erros_5xx_consecutivos >= 10) {
                    erros5xxConsecutivosDetetados++;
                    erros_5xx_consecutivos = 0;
                }
            } else if (log_recebido.status_code > 0 && log_recebido.status_code < 500) { 
                erros_5xx_consecutivos = 0; 
            }

            // Testa Traffic
            if (log_recebido.type == FORMAT_APACHE && log_recebido.status_code == 200) {
                trafego_consecutivo++;
                if (trafego_consecutivo >= 50) {
                    trafefoconsecutivosdetetados++;
                    trafego_consecutivo = 0;
                }
            } else { 
                trafego_consecutivo = 0; 
            }
            break;
    }
    pthread_mutex_unlock(&mutex_attacks);

    // Contagem de avisos (warnings) genéricos
    if (log_recebido.status_code >= 400 && log_recebido.status_code < 500){
        consumeWarnings++;
    } 
    if (log_recebido.type == FORMAT_JSON && log_recebido.level == LOG_WARN){
        consumeWarnings++;
    } 
    if (log_recebido.type == FORMAT_SYSLOG && (log_recebido.is_sudo_attempt || log_recebido.is_firewall_block)){
        consumeWarnings++;
    }
    if (log_recebido.type == FORMAT_NGINX && log_recebido.level == LOG_WARN){
        consumeWarnings++;
    } 

    pthread_mutex_lock(&mutex_cons);
    stats->total_lines++;
    stats->errors += consumeErrors;
    stats->warnings += consumeWarnings;
    pthread_mutex_unlock(&mutex_cons);
    return consumeErrors;
}

void *produtor(void *arg) {
    ProdutorArgs *args = (ProdutorArgs*)arg;
    char line[4096];
    int linhasLidas = 0;
    
    for (int i = 0; i < args->num_ficheiros_atribuidos; i++) {
        char *nome_ficheiro = args->ficheiros_atribuidos[i];
        LogFormat formato_atual = formatCase(nome_ficheiro);
        
        int fd = open(nome_ficheiro, O_RDONLY);
        if (fd == -1) continue;

        while (produz(fd, line) > 0) { 
            linhasLidas++;
            if (linhasLidas % 50 == 0) {
                dashboard_update_thread(args->id, linhasLidas, 0, 0, WORKING); 
            }
            LogEntry logAtual;
            memset(&logAtual, 0, sizeof(LogEntry));
            logAtual.type = formato_atual;
            
            
            switch (formato_atual) {
                case FORMAT_APACHE:{
                    ApacheLogEntry logTemp;
                    if(parse_apache_log(line, &logTemp) == 0){
                        logAtual.status_code = logTemp.status_code;
                        strcpy(logAtual.ip, logTemp.ip);
                    } 
                    break;
                }
                case FORMAT_JSON:{
                    JSONLogEntry logTemp;
                    if(parse_json_log(line, &logTemp) == 0){
                        logAtual.level = logTemp.level;
                        strcpy(logAtual.ip, logTemp.ip);
                    }
                    break;
                }
                case FORMAT_NGINX:{
                    NginxErrorEntry logTemp;
                    if(parse_nginx_error(line, &logTemp) == 0){
                        logAtual.level = logTemp.level;
                        strcpy(logAtual.ip, logTemp.client_ip);
                    }
                    break;
                }
                case FORMAT_SYSLOG:{
                    SyslogEntry logTemp;
                    if(parse_syslog(line, &logTemp) == 0){
                        logAtual.is_auth_failure = logTemp.is_auth_failure;
                        logAtual.is_sudo_attempt = logTemp.is_sudo_attempt;
                        logAtual.is_firewall_block = logTemp.is_firewall_block;
                        strcpy(logAtual.ip, logTemp.hostname);
                    }
                    break;
                }
                default:
                    break;
            }

            sem_wait(&podeProduzir);
            pthread_mutex_lock(&mutex_prod);
            
            buffer[prodptr] = logAtual; // Mete a struct inteira
            prodptr = (prodptr + 1) % MAX_BUFFER;
            
            pthread_mutex_unlock(&mutex_prod);
            sem_post(&podeConsumir);
        }
        close(fd);
    }
    dashboard_update_thread(args->id, linhasLidas, 0, 0, DONE);
    return NULL;
}
    
void *consumidor(void *arg){
    ConsumidorArgs *args = (ConsumidorArgs*)arg;
    int dashboard_id = args->config->numProdutores + args->id;

    
    while (TRUE) {
        LogEntry log_recebido; 
        
        sem_wait(&podeConsumir);
        pthread_mutex_lock(&mutex_cons);
        
        log_recebido = buffer[consptr]; 
        consptr = (consptr + 1) % MAX_BUFFER; // buffer circular
        
        pthread_mutex_unlock(&mutex_cons);
        sem_post(&podeProduzir);
        
        if (log_recebido.type == FORMAT_UNKNOWN && log_recebido.status_code == -1) {
            break; 
        }

        long erros_encontrados = consome(log_recebido, args->config, args->stats); // Passamos a struct
        if (erros_encontrados > 0) {
            dashboard_update_thread(dashboard_id, 0, 0, erros_encontrados, WORKING);
        }
    }
    dashboard_update_thread(dashboard_id, 0, 0, 0, DONE);
    return NULL;
}

void logWorkerProducerConsumer(CONFIG *config){
    int numProd = config->numProdutores;
    int numCons = config->numConsumidores;
    pthread_t tProd[numProd];
    pthread_t tCons[numCons];
    
    sem_init(&podeProduzir, 0, MAX_BUFFER);
    sem_init(&podeConsumir, 0, 0);
    dashboard_init(numProd + numCons, NULL); 
    dashboard_start();
    
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

    dashboard_stop();
    printf("Total de linhas: %ld\nErros: %ld\nAvisos: %ld\n", globalStats.total_lines, globalStats.errors, globalStats.warnings);
    printf("Tentativas de Brute force: %d\nErros HTTP 5xx: %d\nTráfego detetado: %d\n", bruteForcesDetetatos, erros5xxConsecutivosDetetados, trafefoconsecutivosdetetados);
}