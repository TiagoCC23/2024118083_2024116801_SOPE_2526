#include "pipe.h"

int main(){
    int fds[2];

    if(pipe(fds) < 0){ // erro
        perror("ERRO NO PIPE");
    }

    pid_t pid = fork();

    if(pid < 0){ // erro
        perror("ERRO NO FORK");
    } 

    if(pid == 0){ // filho
        close(fds[0]);

        exit(0);

    } else { // pai
        close(fds[1]);
        readn();
    }
}