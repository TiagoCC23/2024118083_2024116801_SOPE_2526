#ifndef LOG_ANALYZER_H
#define LOG_ANALYZER_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "logAnalyzer.h"
#include "event_classifier.h"
#include "log_parser.h"



typedef struct config
{
    char *diretorio;
    int numProcessos;
    int modo;
    int verbose;
    char *outFiles;
} CONFIG;

void parseArguments(int argc, char *argv[], CONFIG *config);
void logWorker(CONFIG *config);

#endif