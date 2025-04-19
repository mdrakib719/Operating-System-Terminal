
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>


#define MAXIMUM_INPUT_LENGTH 1024
#define MAXIMUM_ARGUMENTS 64
#define DELIMITERS " \t\r\n\a"
#define COMMAND_HISTORY_SIZE 10


char *command_history[COMMAND_HISTORY_SIZE];
int current_history_index = 0;

char *SUCCESS_MESSAGE ="\033[32m [SUCCESS]\033[0m";
char *INFORMATION_MESSAGE="\033[36m [INFO]\033[0m";
char *YELLOW_COLOR="\x1b[33m";
char *GREEN_COLOR="\x1b[32m";
char *RESET_COLOR ="\x1b[0m";


void handle_interrupt_signal(int signal_number) {
    if (signal_number==SIGINT) {
        printf("%syes> %s", GREEN_COLOR, RESET_COLOR);
        fflush(stdout);
    }
}


void add_command_to_history(char *command_input) {
    if (command_history[current_history_index]!= NULL) {
        free(command_history[current_history_index]);
    }
    command_history[current_history_index] = strdup(command_input);
    current_history_index = (current_history_index + 1) % COMMAND_HISTORY_SIZE;
}


void handle_input_output_redirection(char **command_arguments) {
    int input_file_descriptor =0,output_file_descriptor = 1;
    char *input_file = NULL,*output_file = NULL;
    int input_redirection= 0,output_redirection = 0, append_redirection =0;

    for (int i = 0; command_arguments[i] != NULL; i++) {
        if (strcmp(command_arguments[i],"<") == 0) {
            input_redirection= 1;
            input_file =command_arguments[i + 1];
            command_arguments[i] = NULL;
        } else if (strcmp(command_arguments[i],">") == 0) {
            output_redirection= 1;
            output_file = command_arguments[i+1];
            command_arguments[i] = NULL;
        } else if (strcmp(command_arguments[i],">>") == 0) {
            append_redirection =1;
            output_file = command_arguments[i + 1];
            command_arguments[i] =NULL;
        }
    }

    if (input_redirection && input_file) {
        input_file_descriptor = open(input_file, O_RDONLY);
        if (input_file_descriptor < 0) {
            perror("Input redirection error");
            exit(1);
        }
        dup2(input_file_descriptor,STDIN_FILENO);
        close(input_file_descriptor);
    }

    if (output_redirection && output_file) {
        output_file_descriptor= open(output_file, O_WRONLY | O_CREAT | O_TRUNC,0644);
        if (output_file_descriptor <0){
            perror("Output redirection error");
            exit(1);
        }
        dup2(output_file_descriptor,STDOUT_FILENO);
        close(output_file_descriptor);
    } else if (append_redirection && output_file) {
        output_file_descriptor =open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (output_file_descriptor <0) {
            perror("Append redirection error");
            exit(1);
        }
        dup2(output_file_descriptor, STDOUT_FILENO);
        close(output_file_descriptor);
    }
}

void parse_command_string(char *command_input, char **command_arguments) {
    char *token= strtok(command_input, DELIMITERS);
    int argument_index = 0;
    while (token !=NULL) {
        command_arguments[argument_index++] = token;
        token = strtok(NULL, DELIMITERS);
    }
    command_arguments[argument_index] = NULL;
}

void execute_single_command(char *command_input) {
    char *command_arguments[MAXIMUM_ARGUMENTS];
    parse_command_string(command_input,command_arguments);
    if (command_arguments[0] == NULL) return;
    
    if (strcmp(command_arguments[0], "cd") == 0) {
        if (command_arguments[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(command_arguments[1]) != 0) {
                perror("cd");
            }
        }
        return;
    }

    pid_t process_id= fork();
    if (process_id==0) {
        handle_input_output_redirection(command_arguments);
        if (execvp(command_arguments[0],command_arguments) == -1) {
            perror("Error executing command");
            exit(1);
        }
    } else if (process_id < 0) {
        perror("Fork failed");
    } else {
        wait(NULL);
    }
}


void execute_piped_command_sequence(char *command_input) {
    char *piped_command_segments[MAXIMUM_ARGUMENTS];
    int number_of_pipes = 0;

    piped_command_segments[number_of_pipes++]=strtok(command_input, "|");
    while ((piped_command_segments[number_of_pipes++]=strtok(NULL, "|")));

    int input_file_descriptor = 0;
    for (int i = 0; i <number_of_pipes-1; i++) {
        int pipe_file_descriptors[2];
        pipe(pipe_file_descriptors);

        pid_t process_id = fork();
        if (process_id == 0) {
            dup2(input_file_descriptor,STDIN_FILENO);
            if (piped_command_segments[i+1]!= NULL) {
                dup2(pipe_file_descriptors[1],STDOUT_FILENO);
            }

            close(pipe_file_descriptors[1]);

            char *command_arguments[MAXIMUM_ARGUMENTS];
            parse_command_string(piped_command_segments[i],command_arguments);
            handle_input_output_redirection(command_arguments);
            if (execvp(command_arguments[0],command_arguments) == -1) {
                perror("Pipe command execution error");
                exit(1);
            }
        }
        close(pipe_file_descriptors[1]);
        input_file_descriptor=pipe_file_descriptors[0];
        wait(NULL);
    }
}


void display_command_history() {
    int count=0;
    for (int i=current_history_index; count<COMMAND_HISTORY_SIZE; i =(i+1)%COMMAND_HISTORY_SIZE) {
        if (command_history[i] != NULL) {
            printf("%d: %s\n", count + 1, command_history[i]);
            count++;
        }
    }
}


void run_shell_loop() {
    char user_input[MAXIMUM_INPUT_LENGTH];
    signal(SIGINT,handle_interrupt_signal);

    while (1) {
        char current_working_directory[1024];
        if (getcwd(current_working_directory, sizeof(current_working_directory)) != NULL) {
            printf("%syes> %s~%s $ %s", GREEN_COLOR, YELLOW_COLOR,current_working_directory, RESET_COLOR);
        } else {
            perror("getcwd");
            printf("%syes> %s",GREEN_COLOR, RESET_COLOR);
        }

        if (!fgets(user_input, MAXIMUM_INPUT_LENGTH, stdin)) break;

        user_input[strcspn(user_input,"\n")] =0;
        if (strlen(user_input) ==0) continue;

        if (strcmp(user_input,"exit") ==0) {
            break;
        }

        if (strcmp(user_input,"history") ==0) {
            display_command_history();
            continue;
        }
        add_command_to_history(user_input);

        if (strchr(user_input, '|')) {
            execute_piped_command_sequence(user_input);
        } else {
            execute_single_command(user_input);
        }
    }

    for (int i =0; i < COMMAND_HISTORY_SIZE; i++) {
        if (command_history[i]) {
            free(command_history[i]);
        }
    }
}

int main() {
    printf("%s all copy right from Rakib\n",INFORMATION_MESSAGE);
    printf("%s \u00A9 Rakib\n", INFORMATION_MESSAGE);
    for (int i=0; i<COMMAND_HISTORY_SIZE; i++) {
        command_history[i] =NULL;
    }
    run_shell_loop();
    return 0;
}
