#ifndef R3_1_LOG_ANALYZER_H
#define R3_1_LOG_ANALYZER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "R3_1_logAnalyzer.h"
#include "event_classifier.h"

typedef struct config
{
    char *diretorio;
    int numProcessos;
    int numThreads;
    int modo;
    int verbose;
    char *outFiles;
} CONFIG;

void parseArguments(int argc, char *argv[], CONFIG *config);

#endif