#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <pthread.h>
#include <time.h>

#include "workers.h"

// ANSI escapes codes
#define ANSI_CLEAR "\033[2J\033[H"
#define ANSI_BOLD "\033[1m"
#define ANSI_RESET "\033[0m"
#define ANSI_CYAN "\033[36m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_GREEN "\033[32m"
#define ANSI_RED "\033[31m"
#define ANSI_WHITE "\033[37m"
#define ANSI_DIM "\033[2m"

#define MAX_WORKERS 32

/* compartilhado entre a thread do dashboard e o loop principal do pai */
WorkerStatus status[MAX_WORKERS];
int g_nworkers = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
/* momento de início (para elapsed/ETA) */
time_t start_time;



typedef struct {
    pid_t pid;
    long lines_processed;
    long total_lines;
    float progress_pct;
    enum {IDLE, WORKING, DONE} state;
} WorkerStatus;

typedef struct {
    long lines_processed;
    long total_lines;
    float progress_pct;
    long errors;
    long warnings;
    int state; // IDLE = 0, WORKING = 1, DONE = 2;
} ProgressMsg; // mensagem de progresso enviada pelo filho

#endif