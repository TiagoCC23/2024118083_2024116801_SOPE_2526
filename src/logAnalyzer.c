#include "logAnalyzer.h"

void parseArguments(int argc, char *argv[], CONFIG *config){
    if (argc<4){
        printf("Formato inválido!!\n");
        fprintf(stderr,"Tente algo como: ./logAnalyzer /var/log/apache2 4 security --verbose --output=report.json --threads=8");
        exit(EXIT_FAILURE);
    }
    config->diretorio = argv[1];
    config->numProcessos = atoi(argv[2]);
    if(config->numProcessos <=0){
        fprintf(stderr,"Tem de haver pelo menos um processo");
        exit(EXIT_FAILURE);
    }
    
    // com base no event_classifier e com o que o utilizador escreveu, os else ifs servem para comparar o input dado com algo diferente que e recebido
    if (strcmp(argv[3], "security") == 0) {
        config->modo = MODE_SECURITY;
    } else if (strcmp(argv[3], "performance") == 0) {
        config->modo = MODE_PERFORMANCE;
    } else if (strcmp(argv[3], "traffic") == 0) {
        config->modo = MODE_TRAFFIC;
    } else if (strcmp(argv[3], "full") == 0) {
        config->modo = MODE_FULL;
    } else {
        fprintf(stderr, "Erro: Modo '%s' invalido.\n", argv[3]);
        exit(EXIT_FAILURE);
    }
    config->verbose = 0;
    config-> outFiles = NULL;


    // ciclo para procurar pelo verbose, pelo output ou pelos threads
    for (int i = 4; i < argc; i++)
    {
        if(strcmp(argv[i], "--verbose") == 0){
            config->verbose=1;
        } else if(strncmp(argv[i], "--output=", 9) == 0){
            config->outFiles = argv[i]+9;
        } else if (strncmp(argv[i], "--threads=", 10) == 0){
            config->numThreads = atoi(argv[i]+10);
        }
    }
    
}

