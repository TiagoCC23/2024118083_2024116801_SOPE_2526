#include "threads.h"
long totalLinesShared = 0;
long totalErrorsShared = 0;
long totalWarningsShared = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


void* threadWorker(void* arg){
    THREADDATA *data = (THREADDATA*)arg;
    long linesWorker=0;
    long errorsWorker=0;
    long warningsWorker=0;
    char currentLine[2048];
    int pos=0;

    //parses
    ApacheLogEntry log_apache;
    JSONLogEntry log_json;
    SyslogEntry log_syslog;
    NginxErrorEntry log_nginx;

    // recebe o ponteiro para o bloco
    long start= data->offset_start;
    long end= data->offset_end;
    char *buffer = data-> buffer_completo;
    // processa as linhas do bloco
    for(long i=start; i<end; i++){
        char c = buffer[i];
        currentLine[pos]=c;
        pos++;
        if(c == '\n'){
            currentLine[pos]='\0';
            //printf("[Thread %d] %s", data->id, currentLine); // descomentar para o debug
            linesWorker++;
    

        switch (data->config->modo)
        {
        case 1:
            if(parse_apache_log(currentLine, &log_apache) == 0) {
                if(log_apache.status_code >= 500){
                        errorsWorker++;
                    } else if(log_apache.status_code >=400){
                        warningsWorker++;
                    }
            }
            break;
        case 2:
            if(parse_json_log(currentLine, &log_json) == 0) {
                    if(log_json.level == LOG_ERROR ||log_json.level == LOG_CRITICAL){
                        errorsWorker++;
                    } else if (log_json.level == LOG_WARN)
                    {
                        warningsWorker++;
                    }
            }
            break;
        case 3:
            if(parse_syslog(currentLine, &log_syslog) == 0) {
                if(log_syslog.is_auth_failure){
                    errorsWorker++;
                } else if(log_syslog.is_sudo_attempt){
                   warningsWorker++;
                } else if (log_syslog.is_firewall_block)
                {
                    warningsWorker++;
                }
            }
            break;
        case 4:
           if(parse_nginx_error(currentLine, &log_nginx) == 0) {
            if(log_nginx.level >= NGINX_ERROR){
                    errorsWorker++;
                    } else if(log_nginx.level == NGINX_WARN){
                        warningsWorker++;
                    }
           }
            break;
        default:
            break;
        }
         pos=0; // limpa a linha
    }
    }
    // atualiza a struct partilhada onde o mutex assegura a segurança
    pthread_mutex_lock(&mutex);
    totalLinesShared += linesWorker;
    totalErrorsShared += errorsWorker;
    totalWarningsShared += warningsWorker;
    pthread_mutex_unlock(&mutex);
    return NULL;
}


void logWorkerThreads(CONFIG *config) {

// descobre os ficheiros .log na pasta
char ficheiros[100][512];
int numFicheiros = listFiles(config->diretorio, ficheiros, 100);
if(numFicheiros <= 0){
    perror("Nenhum ficheiro encontrado");
    exit(EXIT_FAILURE);
}

for(int f=0; f<numFicheiros; f++){
// le todos os ficheiros para dividir em blocos
int fd = open(ficheiros[f], O_RDONLY);
if(fd == -1){
    perror("Erro ao abir o ficheiro ");
    exit(EXIT_FAILURE);
}
struct stat st;
if(fstat(fd, &st) == -1){// permite dividir em blocos, descobrindo o tamanho total
perror("Erro ao ler o ficheiro");
close(fd);
exit(EXIT_FAILURE);
} 
long totalSize = st.st_size; // guarda o tamanho em bytes
if(totalSize == 0){
    perror("Ficheiro vazio  ");
    exit(EXIT_FAILURE);
}
char *buffer = (char*) malloc(totalSize+1); // aloca o buffer para o ficheiro inteiro
long totalReadBytes = 0;
long readedBytes= totalSize;


while (totalReadBytes < totalSize) {
    
    ssize_t byteReaded = read(fd, buffer + totalReadBytes, readedBytes); // le o que falta evitando que ele sobreescreva
    
    if (byteReaded == -1) {
        perror("Erro a ler o ficheiro ");
        free(buffer);
        close(fd);
        exit(EXIT_FAILURE);
    }
    if (byteReaded == 0) {
        break; // EOF
    }
    
   
    totalReadBytes += byteReaded;
    readedBytes -= byteReaded;
}
buffer[totalReadBytes] = '\0';
close(fd);


int workers = config ->numThreads; // pegamos na quantidade de threads e atribuimos aos workers
pthread_t threads[workers];
THREADDATA tData[workers];         // cria-se um array de threads
long currentPos= 0;
// divide os blocos por threads
long chunkSize = totalSize/workers; // calcula o tamanho do bloco do que o thread vai ler
for(int i=0; i<workers; i++){
    tData[i].buffer_completo = buffer;
    tData[i].config = config;
    tData[i].offset_start = currentPos;
    long end = currentPos + chunkSize;
    if(i == (workers-1)){
        tData[i].offset_end = totalSize; // caso seja a ultima thread, nao precisa de procurar pelo \n
    } else {
        while(end < totalSize && buffer[end] !='\n'){
            end++; // enquanto nao chega ao EOF, vamos continuar a avançar na posiçao
        }
        tData[i].offset_end = end+1;
    }
    currentPos = tData[i].offset_end; // atualizmos para a proxima thread
    pthread_create(&threads[i], NULL, threadWorker, &tData[i]); // criaçao da thread
}
for(int i =0; i<workers; i++){
    pthread_join(threads[i],NULL);
}
free(buffer);
}
// relatorio final
printf("Linhas partilhadas: %ld\nErros partilhados: %ld\nAvisos partilhados: %ld\n", totalLinesShared, totalErrorsShared, totalWarningsShared);
}