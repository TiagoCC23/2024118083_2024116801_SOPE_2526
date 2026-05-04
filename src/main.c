#include <stdio.h>
#include "logAnalyzer.h"
#include "workers.h"
#include "threads.h"

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
    logWorker(&config);
    } else if(config.numProcessos == 0 && config.numThreads > 0){
        logWorkerThreads(&config);
    }
    return 0;
}