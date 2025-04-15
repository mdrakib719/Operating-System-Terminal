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