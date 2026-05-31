#include <stdio.h>
#include "R3_1_logAnalyzer.h"
#include "R3_2_workers.h"
#include "R3_3_workers_pipes.h"
#include "R3_4_R4_2_dashboard.h"
#include "R3_5_workers_sockets.h"
#include "R4_1_threads.h"
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
    // Requisito 4.3 - Produtor-Consumidor
    if(config.numProdutores > 0 && config.numConsumidores > 0){
        printf("Produtor Consumidor\n");
        logWorkerProducerConsumer(&config);
    } 
    // Requisitos 4.1 e 4.2 - Worker Threads
    else if(config.numThreads > 0){
        printf("Worker Threads\n");
        logWorkerThreads(&config);
    } 
    // Multi-processamento
    else if(config.numProcessos > 0){
        if (strcmp(config.ipc_mode, "basic") == 0) {
            printf("Multi Processo\n");
            logWorker(&config);
        } else if (strcmp(config.ipc_mode, "pipes") == 0) {
            printf("Comunicação via Pipes\n");
            logWorker_pipes(&config);
        } else if (strcmp(config.ipc_mode, "dashboard") == 0) {
            printf("Dashboard de Processos\n");
            logWorker_dashboard(&config);
        } else if (strcmp(config.ipc_mode, "sockets") == 0) {
            printf("Comunicação via Unix Domain Sockets\n");
            logWorker_sockets(&config);
        } else {
            fprintf(stderr, "Modo IPC '%s' não reconhecido. Opções: basic, pipes, dashboard, sockets.\n", config.ipc_mode);
            return 1;
        }
    } else {
        fprintf(stderr, "Erro de Configuração: Defina num_processos > 0, --threads, ou --produtores/--consumidores.\n");
        return 1;
    }

    return 0;
}