#include <stdio.h>
#include <string.h>
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
    
    if(config.numProdutores > 0 && config.numConsumidores > 0){
        printf("[Execução] Ativando Modelo Produtor-Consumidor...\n");
        logWorkerProducerConsumer(&config);
    } 

    else if(config.numThreads > 0){
        printf("[Execução] Ativando Modelo de Worker Threads...\n");
        logWorkerThreads(&config);
    } 
   
    else if(config.numProcessos > 0){
        if (strcmp(config.ipc_mode, "basic") == 0) {
            printf("Multi-Processo - basic - (Req. B)...\n");
            logWorker(&config);
        } else if (strcmp(config.ipc_mode, "pipes") == 0) {
            printf("Comunicação via Pipes (Req. C)...\n");
            logWorker_pipes(&config);
        } else if (strcmp(config.ipc_mode, "dashboard") == 0) {
            printf("Dashboard de Processos (Req. D)...\n");
            logWorker_dashboard(&config);
        } else if (strcmp(config.ipc_mode, "sockets") == 0) {
            printf("Comunicação via Unix Domain Sockets (Req. E)...\n");
            logWorker_sockets(&config);
        } else {
            perror("Modo IPC '%s' não reconhecido. Opções: basic, pipes, dashboard, sockets.\n");
            return 1;
        }
    } else {
        perror("Erro de Configuração: Defina num_processos > 0, --threads, ou --produtores/--consumidores.\n");
        return 1;
    }
    return 0;
}