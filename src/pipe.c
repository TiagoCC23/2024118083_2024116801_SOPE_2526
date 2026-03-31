#include "pipe.h"

int main(int argc, char **argv){
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

        // falta o trabalho do filho!!!!!!!!!!

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