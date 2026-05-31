#include "R3_4_R4_2_dashboard.h"

static struct WorkerStatus mt_status[MAX_WORKERS];
static long mt_thread_errors[MAX_WORKERS];
static int mt_nworkers = 0;
static long mt_errors = 0;
static long mt_events_sec = 0;
static pthread_mutex_t mt_lock = PTHREAD_MUTEX_INITIALIZER;
static time_t mt_start_time;
static volatile int mt_running = 0;
static pthread_t mt_dash_tid;

static void draw_bar_mt(char *out, int width, float pct) {
    int filled = (int)(pct * (float)width);
    if (filled > width) filled = width;
    out[0] = '\0';

    for (int i = 0; i < width; i++) {
        strcat(out, i < filled ? BLOCO_CHEIO : BLOCO_VAZIO);
    }
}

/**
 * @brief Rotina interna executada pela THREAD dedicada de monitorização.
 */
static void *dashboard_worker_thread_fn(void *arg) {
    long prev_done = 0;
    (void)arg;

    while (mt_running) {
        sleep(1); // Atualização estrita a cada 1 segundo

        pthread_mutex_lock(&mt_lock);

        long cur_done = 0;
        long total_total = 0;
        long total_errors_agora = 0;

        for (int i = 0; i < mt_nworkers; i++) {
            cur_done += mt_status[i].lines_processed;
            total_total += mt_status[i].total_lines;
            total_errors_agora += mt_thread_errors[i];
        }

        mt_events_sec = cur_done - prev_done;
        prev_done = cur_done;
        mt_errors = total_errors_agora;

        time_t now = time(NULL);
        long elapsed = (long)(now - mt_start_time);
        float gpct = (total_total > 0) ? (float)cur_done / (float)total_total : 0.0f;
        long eta = (gpct > 0.0f && gpct < 1.0f) ? (long)((float)elapsed / gpct * (1.0f - gpct)) : 0;

        char buf[1024], bar[64];
        write(STDOUT_FILENO, CLS, strlen(CLS));

        int len = snprintf(buf, sizeof(buf), 
                            CYN BOLD "╔════════════════════════════════════════╗\n║  THREADS LOG ANALYZER - Real-time      ║\n╠════════════════════════════════════════╣\n" RST);
        write(STDOUT_FILENO, buf, len);

        for (int i = 0; i < mt_nworkers; i++) {
            draw_bar_mt(bar, 18, mt_status[i].progress_pct);
            len = snprintf(buf, sizeof(buf), CYN "║ " RST "%sWorker-T %-2d [%s] %3d%%" CYN " ║\n" RST,
                           mt_status[i].state == DONE ? GRN : YEL, i + 1, bar, (int)(mt_status[i].progress_pct * 100));
            write(STDOUT_FILENO, buf, len);
        }

        draw_bar_mt(bar, 18, gpct);
        len = snprintf(buf, sizeof(buf), 
                        CYN "╠════════════════════════════════════════╣\n" RST CYN "║ " RST BOLD "Total Progress: [%s] %3d%%" CYN " ║\n" RST
                        CYN "║ " RST "Events/sec: " YEL "%-6ld" RST " | Errors: " RED "%-4ld" CYN " ║\n" RST
                        CYN "║ " RST "Elapsed: " GRN "%02ld:%02ld" RST " | ETA: " GRN "%02ld:%02ld" 
                        CYN "     ║\n╚════════════════════════════════════════╝\n",
                       bar, (int)(gpct * 100), mt_events_sec, mt_errors, elapsed/60, elapsed%60, eta/60, eta%60);
        write(STDOUT_FILENO, buf, len);

        pthread_mutex_unlock(&mt_lock);
    }
    return NULL;
}

void dashboard_init(int nworkers, long total_lines_per_worker[]) {
    mt_nworkers = nworkers;
    mt_errors = 0;
    mt_events_sec = 0;
    mt_start_time = time(NULL);
    memset(mt_status, 0, sizeof(mt_status));
    for (int i = 0; i < nworkers; i++) {
        mt_status[i].total_lines = total_lines_per_worker ? total_lines_per_worker[i] : 0;
        mt_status[i].state = WORKING;
    }
}

void dashboard_start(void) {
    mt_running = 1;
    pthread_create(&mt_dash_tid, NULL, dashboard_worker_thread_fn, NULL);
}

/**
 * @brief Atualização atómica chamada diretamente pelas Worker Threads do pool produtor-consumidor.
 */
void dashboard_update_thread(int worker_id, long lines_processed, long total_lines, long errors, int state) {
    pthread_mutex_lock(&mt_lock);
    mt_status[worker_id].lines_processed = lines_processed;

    if (total_lines > 0) mt_status[worker_id].total_lines = total_lines;

    mt_status[worker_id].progress_pct = (mt_status[worker_id].total_lines > 0) ? (float)lines_processed / (float)mt_status[worker_id].total_lines : 0.0f;
    mt_status[worker_id].state = state;
    mt_thread_errors[worker_id] = errors;

    pthread_mutex_unlock(&mt_lock);
}

void dashboard_stop(void) {
    mt_running = 0;
    pthread_join(mt_dash_tid, NULL);
}