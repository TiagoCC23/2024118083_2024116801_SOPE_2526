#include "R3_4_R4_2_dashboard.h"
#include "R3_3_workers_pipes.h"

void dashboard_render(struct WorkerStatus *local_status, int nWorkers, time_t start_time, long events_sec, long errors);

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
    long worker_errors[MAX_WORKERS];
    memset(worker_errors, 0, sizeof(worker_errors));

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
            
            dashboard_send_progress(prog_fd, 0, 10000, 0, 1);
            long linhas_lidas = 0;
            long erros_encontrados = 0;

            for (int k = start; k < end; k++) {
                int fd = open(ficheiros[k], O_RDONLY);
                if (fd == -1) continue;

                char line[4096];
                LogFormat formato_atual = formatCase(ficheiros[k]); 

                while (produz(fd, line) > 0) { 
                    linhas_lidas++;
                    
                    LogEntry logAtual;
                    memset(&logAtual, 0, sizeof(LogEntry));
                    logAtual.format = formato_atual;
                    
                    switch (formato_atual) {
                        case FORMAT_APACHE:{
                            ApacheLogEntry logTemp;
                            if(parse_apache_log(line, &logTemp) == 0){
                                logAtual.status_code = logTemp.status_code;
                                strcpy(logAtual.ip, logTemp.ip);
                            } 
                            break;
                        }
                        case FORMAT_JSON:{
                            JSONLogEntry logTemp;
                            if(parse_json_log(line, &logTemp) == 0){
                                logAtual.level = logTemp.level;
                                strcpy(logAtual.ip, logTemp.ip);
                            }
                            break;
                        }
                        case FORMAT_NGINX:{
                            NginxErrorEntry logTemp;
                            if(parse_nginx_error(line, &logTemp) == 0){
                                logAtual.level = logTemp.level;
                                strcpy(logAtual.ip, logTemp.client_ip);
                            }
                            break;
                        }
                        case FORMAT_SYSLOG:{
                            SyslogEntry logTemp;
                            if(parse_syslog(line, &logTemp) == 0){
                                logAtual.is_auth_failure = logTemp.is_auth_failure;
                                logAtual.is_sudo_attempt = logTemp.is_sudo_attempt;
                                logAtual.is_firewall_block = logTemp.is_firewall_block;
                                strcpy(logAtual.ip, logTemp.hostname);
                            }
                            break;
                        }
                        default:
                            break;
                    }

                    // Contagem básica de erros
                    if (logAtual.status_code >= 500) erros_encontrados++;
                    if (logAtual.format == FORMAT_JSON && logAtual.level == LOG_ERROR) erros_encontrados++;
                    if (logAtual.format == FORMAT_SYSLOG && logAtual.is_auth_failure) erros_encontrados++;

                    // Atualiza a dashboard a cada 50 linhas para não entupir o pipe
                    if (linhas_lidas % 50 == 0) {
                        dashboard_send_progress(prog_fd, linhas_lidas, 10000, erros_encontrados, 1);
                        usleep(1000); 
                    }
                }
                close(fd);
            }
            dashboard_send_progress(prog_fd, linhas_lidas, linhas_lidas, erros_encontrados, 2);
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
                worker_errors[i] = pm.errors;

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
            long total_errors_now = 0;
            for (int i = 0; i < nWorkers; i++){ 
                cur_done += local_status[i].lines_processed;
                total_errors_now += worker_errors[i];
            }
            events_sec = cur_done - prev_done;
            prev_done = cur_done;
            last_draw = now;
        dashboard_render(local_status, nWorkers, start_time, events_sec, total_errors_now);
        }
        usleep(10000); // Evitar consumo excessivo de CPU no polling de não-bloqueio
    }

    for (int i = 0; i < nWorkers; i++) waitpid(pids[i], NULL, 0);
}
void dashboard_render(struct WorkerStatus *local_status, int nWorkers, time_t start_time, long events_sec, long errors) {
            time_t now = time(NULL);
            long cur_done = 0;
            long total_total = 0;
            for (int i = 0; i < nWorkers; i++) {
                cur_done += local_status[i].lines_processed;
                total_total += local_status[i].total_lines;
            }
            

            float gpct = (total_total > 0) ? (float)cur_done / (float)total_total : 0.0f;
            long elapsed = (now - start_time);
            long eta = (gpct > 0.0f && gpct < 1.0f) ? (long)((float)elapsed / gpct * (1.0f - gpct)) : 0;

            // RENDER VISUAL EM TERMINAL
            char buf[1024], bar[64];
            write(STDOUT_FILENO, CLS, strlen(CLS));
            
            int len = snprintf(buf, sizeof(buf), CYN BOLD "╔═══════════════════════════════════════════╗\n║    LOG ANALYZER - Real-time Monitor    ║\n╠═══════════════════════════════════════════╣\n" RST);
            write(STDOUT_FILENO, buf, len);

            for (int i = 0; i < nWorkers; i++) {
                draw_bar(bar, 18, local_status[i].progress_pct);
                len = snprintf(buf, sizeof(buf), CYN "║ " RST "%sWorker %-2d [%s] %3d%%" CYN " ║\n" RST,
                               local_status[i].state == DONE ? GRN : YEL, i + 1, bar, (int)(local_status[i].progress_pct * 100));
                write(STDOUT_FILENO, buf, len);
            }

            draw_bar(bar, 18, gpct);
            len = snprintf(buf, sizeof(buf), CYN "╠═══════════════════════════════════════════╣\n" RST CYN "║ " RST BOLD "Total Progress: [%s] %3d%%" CYN " ║\n" RST
                           CYN "║ " RST "Events/sec: " YEL "%-6ld" RST " | Errors: " RED "%-4ld" CYN " ║\n" RST
                           CYN "║ " RST "Elapsed: " GRN "%02ld:%02ld" RST " | ETA: " GRN "%02ld:%02ld" CYN "     ║\n╚═══════════════════════════════════════════╝\n",
                           bar, (int)(gpct * 100), events_sec, errors, elapsed/60, elapsed%60, eta/60, eta%60);
            write(STDOUT_FILENO, buf, len);
}