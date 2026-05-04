#ifndef WORKERS_H
#define WORKERS_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include "logAnalyzer.h"
#include "event_classifier.h"
#include <dirent.h>
#include "log_parser.h"

typedef struct {
    pid_t pid;
    long total_lines;
    long errors;
    long warnings;
    char top_ip[16]; 
    int is_event; // 0 = resultado final; 1 = evento em tempo real
    char timestamp[32];
    char type[16];
    char message[256]; // menssagem a ser escrita
    char ip[16];
} PipeMessage;

void logWorker(CONFIG *config);
void workersLogic(int fd_leitura, int id, CONFIG *config, int numFIles);
int listFiles(const char *diretorio, char ficheiros[][512], int maxFicheiros);

#endif