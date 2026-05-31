#include "R3_5_workers_sockets.h"
#include "R3_4_R4_2_dashboard.h"

void logWorker_sockets(CONFIG *config){
    int nWorkers = config->numProcessos;
 
    char ficheiros[100][512];
    int numFicheiros = listFiles(config->diretorio, ficheiros, 100);
    if (numFicheiros <= 0) {
        write(STDOUT_FILENO, "Nenhum ficheiro .log encontrado.\n", 33);
        return;
    }

    // PAI (SERVIDOR)
    int server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("ERRO: Falha ao criar o socket do servidor");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un server_addr;
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, "/tmp/loganalyzer.sock", sizeof(server_addr.sun_path) - 1);

    // apaga o ficheiro do socket se ele já existir de uma execução anterior
    unlink("/tmp/loganalyzer.sock");

    // associa o socket ao caminho no disco
    if (bind(server_sock, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_un)) == -1) {
        perror("ERRO: Falha no bind");
        exit(EXIT_FAILURE);
    }

    // aceita conexões de entrada (de socket ativo passa para passivo)
    if (listen(server_sock, nWorkers) == -1) {
        perror("ERRO: Falha no listen");
        exit(EXIT_FAILURE);
    }

    pid_t pids[nWorkers];
    int arquivos_por_filho = numFicheiros / nWorkers;
    int remain = numFicheiros % nWorkers;
    int start = 0; 
    int prog_pipes[nWorkers][2];
    int done_prog[nWorkers];
    memset(done_prog, 0, sizeof(done_prog));
 
    for (int i = 0; i < nWorkers; i++) {
        int count = arquivos_por_filho + (i < remain ? 1 : 0);
        int end = start + count;
 
        if (pipe(prog_pipes[i]) == -1) {
        perror("pipe progresso");
        exit(EXIT_FAILURE);
        }

        pids[i] = fork();
 
        if (pids[i] == -1) {
            perror("ERRO NO FORK");
            exit(EXIT_FAILURE);
        }
 

        if (pids[i] == 0) { 
            // FILHO (CLIENTE)
            int client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
            if (client_sock == -1) {
                perror("ERRO: Filho falhou ao criar socket");
                exit(EXIT_FAILURE);
            }
            

            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(struct sockaddr_un));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, "/tmp/loganalyzer.sock", sizeof(addr.sun_path) - 1);

            if (connect(client_sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
                perror("ERRO: Filho falhou ao conectar ao servidor");
                exit(EXIT_FAILURE);
            }
            // fecha os lados de leitura dos prog_pipes de todos os workers
            for (int j = 0; j < nWorkers; j++){
             close(prog_pipes[j][0]);
            }
            filho_logic(client_sock, prog_pipes[i][1], i, config, ficheiros, start, end);
        }
 
        start = end; 

        // fecha a escrita e previne que o processo fique parado á espera de dados para ler (leitura não bloqueante)
        close(prog_pipes[i][1]);
        fcntl(prog_pipes[i][0], F_SETFL, O_NONBLOCK);
    }
 
    int client_fds[nWorkers];
    for (int i = 0; i < nWorkers; i++) {
        // O accept bloqueia até que um filho faça o connect()
        client_fds[i] = accept(server_sock, NULL, NULL);
        if (client_fds[i] == -1) {
            perror("ERRO: Pai falhou no accept");
            exit(EXIT_FAILURE);
        }
    }

    long total_lines   = 0;
    long total_errors  = 0;
    long total_warnings = 0;
    int workers_active = nWorkers;
    
    int done[nWorkers];
    memset(done, 0, sizeof(done)); 
 
    // dashboard
    struct WorkerStatus ws[MAX_WORKERS];
    time_t start_time = time(NULL);
    time_t last_draw = 0;
    long prev_done = 0;
    long events_sec = 0;
    long static_errors = 0;

    for (int i = 0; i < nWorkers; i++) {
        ws[i].pid = pids[i];
        ws[i].lines_processed = 0;
        ws[i].total_lines = 10000;
        ws[i].progress_pct = 0.0f;
        ws[i].state = WORKING;
    }
    while (workers_active > 0) {
        for (int i = 0; i < nWorkers; i++) {
            if (done[i]) continue; 
 
                ProgressMsg pm;
                ssize_t np = read(prog_pipes[i][0], &pm, sizeof(pm));
            if (np > 0) {
                ws[i].lines_processed = pm.lines_processed;
                ws[i].total_lines     = pm.total_lines;
                ws[i].progress_pct    = pm.progress_pct;
                ws[i].state           = pm.state;
                static_errors         = pm.errors;
            }
            Message msg;
            // leitura do socket do cliente sem bloquear
            ssize_t n = recv(client_fds[i], &msg, sizeof(msg), MSG_DONTWAIT);
 
            if (n <= 0){
                if(n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                done[i] = 1;
                workers_active--;
                close(client_fds[i]); // fechamos o socket
                } 
             continue;
            }
 
            if (msg.type == MSG_TYPE_NORMAL && msg.size == sizeof(NormalMsg)) {
                NormalMsg norm_msg;
                readn(client_fds[i], &norm_msg, sizeof(norm_msg));
 
                total_lines += norm_msg.total_lines;
                total_errors += norm_msg.errors;
                total_warnings += norm_msg.warnings;
 
                char buf[256];
                int len = snprintf(buf, sizeof(buf),
                    "[Worker %d] PID:%d | Linhas:%ld | Erros:%ld | Avisos:%ld | Top IP:%s\n",
                    i, norm_msg.pid, norm_msg.total_lines, norm_msg.errors, norm_msg.warnings, norm_msg.top_ip);
                write(STDOUT_FILENO, buf, len);
 
            } else if (msg.type == MSG_TYPE_VERBOSE && msg.size == sizeof(VerboseMsg)) {
                VerboseMsg verb_msg;
                readn(client_fds[i], &verb_msg, sizeof(verb_msg));
 
                char buf[512];
                int  len = snprintf(buf, sizeof(buf),
                    "[VERBOSE] PID:%d | %s | %s | IP:%s | %s\n",
                    verb_msg.pid, verb_msg.timestamp, verb_msg.type, verb_msg.ip, verb_msg.msg);
                write(STDOUT_FILENO, buf, len);
 
            } else if (msg.type == MSG_TYPE_DONE) {
                done[i] = 1;
                workers_active--;
                close(client_fds[i]); // fechamos o socket
            }
        }
        time_t now = time(NULL);
        if (now - last_draw >= 1) {
            long cur_done = 0;
            for (int i = 0; i < nWorkers; i++) cur_done += ws[i].lines_processed;
            events_sec = cur_done - prev_done;
            prev_done  = cur_done;
            last_draw  = now;
            dashboard_render(ws, nWorkers, start_time, events_sec, static_errors);
        } 
usleep(10000);
    }
 
    for (int i = 0; i < nWorkers; i++) {
        waitpid(pids[i], NULL, 0);
    }
 
    char report[512];
    int  rlen = snprintf(report, sizeof(report),
        "\n>>>>>> RELATÓRIO FINAL <<<<<<\n"
        "Ficheiros processados: %d\n"
        "Total de linhas: %ld\n"
        "Total de erros: %ld\n"
        "Total de avisos: %ld\n"
        "\n",
        numFicheiros, total_lines, total_errors, total_warnings);
    write(STDOUT_FILENO, report, rlen);

    close(server_sock);
    unlink("/tmp/loganalyzer.sock"); // limpa o ficheiro finalizado
}