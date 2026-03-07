#include <stdio.h>
#include "logAnalyzer.h"

int main(int argc, char *argv[]){
    CONFIG config;

    parseArguments(argc, argv, &config);

    printf("Parse Arguments configurado com sucesso");

    printf("Diretório atual: %s\n",config.diretorio);
    printf("Número de processos: %d\n",config.numProcessos);
    printf("Modo escolhido: %d\n",config.modo);
    printf("Modo Verboso: %s\n", config.verbose ? "Ligado" : "Desligado");
    if(config.outFiles){
        printf("A guardar ficheiro em: %s\n", config.outFiles);
    }
    return 0;
}