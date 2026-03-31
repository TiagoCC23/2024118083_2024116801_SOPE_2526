#include <unistd.h>     // pipe, fork, read, write, close
#include <sys/types.h>  // pid_t, ssize_t
#include <sys/wait.h>   // wait, waitpid
#include <fcntl.h>      // open, O_RDONLY
#include <stdio.h>      // perror
#include <stdlib.h>     // exit, EXIT_FAILURE
#include <errno.h>      // errno, EINTR
#include <string.h>     // strcmp, strlen

