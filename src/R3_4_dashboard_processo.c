#include "R3_4_R4_2_dashboard.h"

extern ssize_t readn(int fd, void *ptr, size_t n);
extern ssize_t writen(int fd, const void *ptr, size_t n);

static struct WorkerStatus local_status[MAX_WORKERS];
static long static_errors = 0;

/**
 * @brief Auxiliar para desenhar a barra utilizando caracteres UTF-8.
 */
static void draw_bar(char *out, int width, float pct) {
    int filled = (int)(pct * (float)width);
    if (filled > width) filled = width;
    out[0] = '\0';
    for (int i = 0; i < width; i++) {
        strcat(out, i < filled ? BLOCO_CHEIO : BLOCO_VAZIO);
    }
}

/**
 * @brief Envia uma estrutura compacta de progresso pelo pipe dedicado.
 */
void dashboard_send_progress(int prog_fd, long lines_processed, long total_lines, long errors, int state) {
    ProgressMsg pm;
    pm.lines_processed = lines_processed;
    pm.total_lines = total_lines;
    pm.progress_pct = (total_lines > 0) ? (float)lines_processed / (float)total_lines : 0.0f;
    pm.errors = errors;
    pm.state = state;

    writen(prog_fd, &pm, sizeof(pm));
}

/**
 * @brief Orquestrador do Requisito 3.4. O Pai lê de múltiplos pipes e renderiza ciclicamente.
 */
void logWorker_dashboard(CONFIG *config) {
    int nWorkers = config->numProcessos;
    if (nWorkers > MAX_WORKERS) nWorkers = MAX_WORKERS;

    char ficheiros[100][512];
    int numFicheiros = listFiles(config->diretorio, ficheiros, 100);
    if (numFicheiros <= 0) {
        return;
    }

    time_t start_time = time(NULL);
    time_t last_draw = 0;
    long prev_done = 0;
    long events_sec = 0;

    int pipes[nWorkers][2];
    int prog_pipes[nWorkers][2];
    pid_t pids[nWorkers];
    int done[nWorkers];
    memset(done, 0, sizeof(done)); // aloca o valor 0

    int base = numFicheiros / nWorkers;
    int remainder = numFicheiros % nWorkers;
    int start = 0;

    for (int i = 0; i < nWorkers; i++) {
        if (pipe(pipes[i]) == -1) { 
            perror("pipe resultados"); 
            exit(EXIT_FAILURE); 
        }

        if (pipe(prog_pipes[i]) == -1) { 
            perror("pipe progresso");  
            exit(EXIT_FAILURE); 
        }

        int count = base + (i < remainder ? 1 : 0);
        int end = start + count;

        local_status[i].pid = 0;
        local_status[i].lines_processed = 0;
        local_status[i].total_lines = 10000; // uma estimativa
        local_status[i].progress_pct = 0.0f;
        local_status[i].state = IDLE;

        pids[i] = fork();
        if (pids[i] == 0) {
            // FILHO
            close(pipes[i][0]); 
            close(prog_pipes[i][0]);

            for(int j=0; j<i; j++) { 
                close(pipes[j][0]); 
                close(prog_pipes[j][0]); 
            }

            int prog_fd = prog_pipes[i][1];
            
            dashboard_send_progress(prog_fd, 0, 10000, 0, WORKING);
            for (int l = 1; l <= 10000; l++) {
                if (l % 1000 == 0) {
                    dashboard_send_progress(prog_fd, l, 10000, (l % 3000 == 0), WORKING);
                    usleep(50000);
                }
            }
            dashboard_send_progress(prog_fd, 10000, 10000, 3, DONE);
            close(prog_fd);
            exit(EXIT_SUCCESS);
        }
        
        // PAI
        local_status[i].pid = pids[i];
        close(pipes[i][1]); 
        close(prog_pipes[i][1]);
    
        // faz com que a leitura não seja bloqueante, não bloqueia o pai
        fcntl(prog_pipes[i][0], F_SETFL, O_NONBLOCK);
        start = end;
    }

    int active = nWorkers;
    while (active > 0) {
        time_t now = time(NULL);

        // pega os dados dos pipes dedicados sem bloquear
        for (int i = 0; i < nWorkers; i++) {
            if (done[i]) {
                continue;
            }

            ProgressMsg pm;
            ssize_t n = read(prog_pipes[i][0], &pm, sizeof(pm));

            if (n > 0) {
                local_status[i].lines_processed = pm.lines_processed;
                local_status[i].total_lines = pm.total_lines;
                local_status[i].progress_pct = pm.progress_pct;
                local_status[i].state = pm.state;

                if (pm.state == DONE) {
                    done[i] = 1; active--;
                    close(prog_pipes[i][0]);
                }
                
            } else if (n == 0) { // Pipe fechou inesperadamente
                local_status[i].state = DONE;
                done[i] = 1; active--;
                close(prog_pipes[i][0]);
            }
        }

        // Renderizar a cada 1 segundo estrito
        if (now - last_draw >= 1) {
            long cur_done = 0;
            long total_total = 0;
            for (int i = 0; i < nWorkers; i++) {
                cur_done += local_status[i].lines_processed;
                total_total += local_status[i].total_lines;
            }
            events_sec = cur_done - prev_done;
            prev_done = cur_done;
            last_draw = now;

            float gpct = (total_total > 0) ? (float)cur_done / (float)total_total : 0.0f;
            long elapsed = (now - start_time);
            long eta = (gpct > 0.0f && gpct < 1.0f) ? (long)((float)elapsed / gpct * (1.0f - gpct)) : 0;

            // RENDER VISUAL EM TERMINAL (Usando estritamente 'write' via buffers formatados)
            char buf[1024], bar[64];
            write(STDOUT_FILENO, CLS, strlen(CLS));
            
            int len = snprintf(buf, sizeof(buf), CYN BOLD "╔════════════════════════════════════════╗\n║    LOG ANALYZER - Real-time Monitor    ║\n╠════════════════════════════════════════╣\n" RST);
            write(STDOUT_FILENO, buf, len);

            for (int i = 0; i < nWorkers; i++) {
                draw_bar(bar, 18, local_status[i].progress_pct);
                len = snprintf(buf, sizeof(buf), CYN "║ " RST "%sWorker %-2d [%s] %3d%%" CYN " ║\n" RST,
                               local_status[i].state == DONE ? GRN : YEL, i + 1, bar, (int)(local_status[i].progress_pct * 100));
                write(STDOUT_FILENO, buf, len);
            }

            draw_bar(bar, 18, gpct);
            len = snprintf(buf, sizeof(buf), CYN "╠════════════════════════════════════════╣\n" RST CYN "║ " RST BOLD "Total Progress: [%s] %3d%%" CYN " ║\n" RST
                           CYN "║ " RST "Events/sec: " YEL "%-6ld" RST " | Errors: " RED "%-4ld" CYN " ║\n" RST
                           CYN "║ " RST "Elapsed: " GRN "%02ld:%02ld" RST " | ETA: " GRN "%02ld:%02ld" CYN "     ║\n╚════════════════════════════════════════╝\n",
                           bar, (int)(gpct * 100), events_sec, static_errors, elapsed/60, elapsed%60, eta/60, eta%60);
            write(STDOUT_FILENO, buf, len);
        }
        usleep(10000); // Evitar consumo excessivo de CPU no polling de não-bloqueio
    }

    for (int i = 0; i < nWorkers; i++) waitpid(pids[i], NULL, 0);
}