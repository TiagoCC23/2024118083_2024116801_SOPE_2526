#include <stdio.h>
#include "R3_1_logAnalyzer.h"
#include "R3_2_workers.h"
#include "R4_1_threads.h"
#include "R3_3_workers_pipes.h"
#include "R4_3_prodConsBoundedBuffer.h"

int main(int argc, char *argv[]){
    CONFIG config;

    parseArguments(argc, argv, &config);

    printf("Parse Arguments configurado com sucesso");

    printf("Diretório atual: %s\n",config.diretorio);
    printf("Número de processos: %d\n",config.numProcessos);
    printf("Número de tarefas: %d\n", config.numThreads);
    printf("Modo escolhido: %d\n",config.modo);
    printf("Modo Verboso: %s\n", config.verbose ? "Ligado" : "Desligado");
    if(config.outFiles){
        printf("A guardar ficheiro em: %s\n", config.outFiles);
    }
    if(config.numProcessos > 0 && config.numThreads == 0){
    logWorker_pipes(&config); // ou logWorker(&config); 
    } else if(config.numThreads > 0){
        logWorkerThreads(&config);
    }
    if(config.numProdutores > 0 && config.numConsumidores > 0){
    //logWorkerProducerConsumer(&config);
    }
    return 0;
}