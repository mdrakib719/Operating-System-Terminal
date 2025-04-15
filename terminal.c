#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define DELIM " \t\r\n\a"
#define HISTORY_SIZE 10

char *history[HISTORY_SIZE];
int history_index = 0;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\nmy-mini-shell> ");
        fflush(stdout);
    }
}
