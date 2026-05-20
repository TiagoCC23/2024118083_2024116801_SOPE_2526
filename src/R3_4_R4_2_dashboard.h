#ifndef R3_4_R4_2_DASHBOARD_H
#define R3_4_R4_2_DASHBOARD_H

#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <sys/select.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h> // para 4.2

#include "R3_2_workers.h"

#define CLS  "\033[2J\033[H"
#define BOLD "\033[1m"
#define RST  "\033[0m"
#define CYN  "\033[36m"
#define GRN  "\033[32m"
#define YEL  "\033[33m"
#define RED  "\033[31m"
#define DIM  "\033[2m"

#define BLOCO_CHEIO "\xe2\x96\x88" /* █ */
#define BLOCO_VAZIO "\xe2\x96\x91" /* ░ */

#define MAX_WORKERS 32

/**
 * @struct WorkerStatus
 * @brief Estrutura exata pedida no enunciado para rastrear o estado de cada worker.
 */
struct WorkerStatus {
    pid_t pid;              /**< Process ID (ou Thread ID interpretado) */
    long  lines_processed;   /**< Linhas processadas até ao momento */
    long  total_lines;       /**< Total de linhas a processar */
    float progress_pct;     /**< Percentagem de progresso (0.0 a 1.0) */
    enum { IDLE, WORKING, DONE } state; /**< Estado atual do worker */
};

/**
 * @struct ProgressMsg
 * @brief Mensagem binária compacta enviada via pipe dedicado pelo processo filho (Requisito 3.4).
 */
typedef struct {
    long  lines_processed;
    long  total_lines;
    float progress_pct;
    long  errors;
    int   state;
} ProgressMsg;

// 3.4
void logWorker_dashboard(CONFIG *config);
void dashboard_send_progress(int prog_fd, long lines_processed, long total_lines, long errors, int state);

// 4.2
void dashboard_init(int nworkers, long total_lines_per_worker[]);
void dashboard_start(void);
void dashboard_update_thread(int worker_id, long lines_processed, long total_lines, long errors, int state);
void dashboard_stop(void);

#endif