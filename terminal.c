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

void add_to_history(char *cmd) {
    if (history[history_index] != NULL) {
        free(history[history_index]);
    }
    history[history_index] = strdup(cmd);
    history_index = (history_index + 1) % HISTORY_SIZE;
}

void handle_redirection(char **args) {
    int input_fd = 0, output_fd = 1;
    char *infile = NULL, *outfile = NULL;
    int in_redirect = 0, out_redirect = 0, append_redirect = 0;

    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            in_redirect = 1;
            infile = args[i + 1];
            args[i] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            out_redirect = 1;
            outfile = args[i + 1];
            args[i] = NULL;
        } else if (strcmp(args[i], ">>") == 0) {
            append_redirect = 1;
            outfile = args[i + 1];
            args[i] = NULL;
        }
    }

    if (in_redirect && infile) {
        input_fd = open(infile, O_RDONLY);
        if (input_fd < 0) {
            perror("Input redirection error");
            exit(1);
        }
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

    if (out_redirect && outfile) {
        output_fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
            perror("Output redirection error");
            exit(1);
        }
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    } else if (append_redirect && outfile) {
        output_fd = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (output_fd < 0) {
            perror("Append redirection error");
            exit(1);
        }
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
}

void parse_command(char *cmd, char **args) {
    char *token = strtok(cmd, DELIM);
    int i = 0;
    while (token != NULL) {
        args[i++] = token;
        token = strtok(NULL, DELIM);
    }
    args[i] = NULL;
}

void execute_command(char *cmd) {
    char *args[MAX_ARGS];
    parse_command(cmd, args);
    if (args[0] == NULL) return;

    pid_t pid = fork();
    if (pid == 0) {
        handle_redirection(args);
        if (execvp(args[0], args) == -1) {
            perror("Error executing command");
            exit(1);
        }
    } else if (pid < 0) {
        perror("Fork failed");
    } else {
        wait(NULL);
    }
}

void execute_piped_commands(char *cmd) {
    char *pipe_cmds[MAX_ARGS];
    int pipe_count = 0;

    pipe_cmds[pipe_count++] = strtok(cmd, "|");
    while ((pipe_cmds[pipe_count++] = strtok(NULL, "|")));

    int input_fd = 0;
    for (int i = 0; i < pipe_count - 1; i++) {
        int fd[2];
        pipe(fd);

        pid_t pid = fork();
        if (pid == 0) {
            dup2(input_fd, 0);
            if (pipe_cmds[i + 1] != NULL) {
                dup2(fd[1], 1);
            }

            close(fd[1]);

            char *args[MAX_ARGS];
            parse_command(pipe_cmds[i], args);
            handle_redirection(args);
            if (execvp(args[0], args) == -1) {
                perror("Pipe command execution error");
                exit(1);
            }
        }
        close(fd[1]);
        input_fd = fd[0];
        wait(NULL);
    }
}

void print_history() {
    int count = 0;
    for (int i = history_index; count < HISTORY_SIZE; i = (i + 1) % HISTORY_SIZE) {
        if (history[i] != NULL) {
            printf("%d: %s\n", count + 1, history[i]);
            count++;
        }
    }
}

void shell_loop() {
    char input[MAX_INPUT];
    signal(SIGINT, handle_signal);

    while (1) {
        printf("my-mini-shell> ");
        if (!fgets(input, MAX_INPUT, stdin)) break;

        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;

        if (strcmp(input, "exit") == 0) {
            break;
        }

        if (strcmp(input, "history") == 0) {
            print_history();
            continue;
        }

        add_to_history(input);

        if (strchr(input, '|')) {
            execute_piped_commands(input);
        } else {
            execute_command(input);
        }
    }

    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (history[i]) {
            free(history[i]);
        }
    }
}

int main() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history[i] = NULL;
    }
    shell_loop();
    return 0;
}
