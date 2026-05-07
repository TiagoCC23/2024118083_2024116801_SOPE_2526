#ifndef R3_4_DASHBOARD_H
#define R3_4_DASHBOARD_H

#include <pthread.h>
#include <time.h>

#include "R3_2_workers.h"

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