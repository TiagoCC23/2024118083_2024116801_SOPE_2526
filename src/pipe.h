#include <unistd.h>     // pipe, fork, read, write, close
#include <sys/types.h>  // pid_t, ssize_t
#include <sys/wait.h>   // wait, waitpid
#include <fcntl.h>      // open, O_RDONLY
#include <stdio.h>      // perror
#include <stdlib.h>     // exit, EXIT_FAILURE
#include <errno.h>      // errno, EINTR
#include <string.h>     // strcmp, strlen
#include <sys/types.h>  // para pid_t

typedef struct {
    pid_t pid;
    long total_lines;
    long errors;
    long warnings;
    char top_ip[16]; 
    int is_event; // 0 = resultado final; 1 = evento em tempo real
    char timestamp[32];
    char type[16];
    char message[256]; // menssagem a ser escrita
    char ip[16];
} PipeMessage;

typedef struct {
    pid_t pid;
    long lines_processed;
    long total_lines;
} ProgressUpdate;

ssize_t readn(int fd, void *ptr, size_t n); 
ssize_t writen(int fd, const void *ptr, size_t n);