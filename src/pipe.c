#include "pipe.h"

ssize_t writen(int fd, const void *ptr, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char *bufptr = ptr;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufptr, nleft)) <= 0) {
            if (nwritten < 0 && errno == EINTR) nwritten = 0; 
            else return -1;
        }
        nleft -= nwritten;
        bufptr += nwritten;
    }
    return n;
}

ssize_t readn(int fd, void *ptr, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *bufptr = ptr;

    while (nleft > 0) {
        if ((nread = read(fd, bufptr, nleft)) < 0) {
            if (errno == EINTR) nread = 0; 
            else return -1;
        } else if (nread == 0) {
            break; // EOF (tubo fechado pelo outro lado)
        }
        nleft -= nread;
        bufptr += nread;
    }
    return (n - nleft); 
}

int main(int argc, char **argv){
    if (argc < 3) {
        char *uso = "Uso: ./programa <ficheiro.log> <modo_verbose(0 ou 1)>\n";
        write(STDOUT_FILENO, uso, strlen(uso));
        exit(1);
    }

    char *file = argv[1];
    int modo_verbose = atoi(argv[2]); // se for 1 é verbose

    int fds[2];

    if(pipe(fds) < 0){ // erro
        perror("ERRO NO PIPE");
        exit(1); // saída de erro
    }

    pid_t pid = fork();

    if(pid < 0){ // erro
        perror("ERRO NO FORK");
        exit(1); // saída de erro
    } 

    if(pid == 0){ // filho
        close(fds[0]);

        int log = open(file, O_RDONLY);
        if(log < 0){
            perror("ERRO NA ABERTURA DO LOG");
            close(fds[1]);
            exit(1);
        }

        PipeMessage msg;
        memset(&msg, 0, sizeof(PipeMessage)); // limpa para não mandar lixo pro pipe

        msg.pid = getpid();

        char line[2048]; // buffer para guardar a linha atual
        int pos = 0; // posicao atual na linha
        char c; // guarda o byte lido

        while(read(log, &c, 1) > 0){
            if(c == '\n' || pos >= sizeof(linha) - 1){
                linha[pos] = '\0';
                msg.total_lines++;

                if(strstr(line, "ERROR")){
                    msg.errors++;

                    if(modo_verbose == 1){

                    }
                }
            }
        }

        close(fds[1]);
        exit(0); // saída de sucesso

    } else { // pai
        close(fds[1]); // fecha o lado de escrita

        PipeMessage msg;
        long total_lines = 0, total_errors = 0, total_warnings = 0;

        while(readn(fds[0], &msg, sizeof(PipeMessage)) > 0){ // o readn() é melhor que o read() porque devolve todos os bytes pedidos!!!
            if(msg.is_event == 1){ // modo: verbose
                write(STDOUT_FILENO, "»»» EVENTO CRÍTICO «««", 17);
                // falta imprimir os campos do evento!!!!!!!!!!
            } else { // modo: normal
                total_lines += msg.total_lines;
                total_errors += msg.errors;
                total_warnings += msg.warnings;
            }
        }

        close(fds[0]);
        // falta imprimir o relatorio final!!!!!!!!!!
    }
}