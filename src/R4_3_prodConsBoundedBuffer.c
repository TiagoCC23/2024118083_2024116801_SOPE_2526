#include "R4_3_prodConsBoundedBuffer.h"

pthread_mutex_t mutex_prod = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cons = PTHREAD_MUTEX_INITIALIZER;
sem_t podeProduzir;
sem_t podeConsumir;
int prodptr = 0, constptr = 0;
//provisório
#define N 10
typedef struct {
    char linha[4096];  // a linha de log
    int  ocupado;      // 0 = vazio, 1 = tem dados
} LogEntry;

LogEntry buffer[N];


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
  return 0 // EOF

}
void consome(int item){
    sleep(1);
}



void *produtor(void *arg){
    while (TRUE) {
        int item = produz();

        sem_wait(&podeProduzir);

        // Tranca APENAS a zona de produção
        pthread_mutex_lock(&mutex_prod); 
        buffer[prodptr] = item;
        prodptr = (prodptr + 1) % N;
        pthread_mutex_unlock(&mutex_prod);

        sem_post(&podeConsumir);
    }
    return NULL;
}
    
void *consumidor(void *arg){
    while (TRUE) {
        int item;

        sem_wait(&podeConsumir);

        pthread_mutex_lock(&mutex_cons); 
        item = buffer[constptr]; 
        buffer[constptr] = 0; 
        constptr = (constptr + 1) % N; 
        pthread_mutex_unlock(&mutex_cons);

        sem_post(&podeProduzir); 

        consome(item);
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
    argsProd[i].config = config;
    strcpy(argsProd[i].ficheiro, ficheiros[i]);
    argsProd[i].id = i;
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

}