#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_HISTORY 100
#define MAX_CMDS 16  

char *args[MAX_ARGS];

// Structure to store detailed history info (for part h)
struct HistoryEntry {
    char command[MAX_INPUT];
    pid_t pid;
    time_t start_time;
    double duration;
};

struct HistoryEntry history[MAX_HISTORY];
int history_count = 0;


// Store command entry with details
void add_to_history(const char *command, pid_t pid, time_t start, double duration) {
    if (history_count < MAX_HISTORY) {
        history[history_count].pid = pid;
        history[history_count].start_time = start;
        history[history_count].duration = duration;
        strncpy(history[history_count].command, command, MAX_INPUT);
        history[history_count].command[MAX_INPUT - 1] = '\0';
        history_count++;
    } else {
        // shift old entries if max reached
        for (int i = 1; i < MAX_HISTORY; i++) {
            history[i - 1] = history[i];
        }
        history[MAX_HISTORY - 1].pid = pid;
        history[MAX_HISTORY - 1].start_time = start;
        history[MAX_HISTORY - 1].duration = duration;
        strncpy(history[MAX_HISTORY - 1].command, command, MAX_INPUT);
    }
}

// Show only command strings (for "history" command — part g)
void show_history_commands() {
    for (int i = 0; i < history_count; i++) {
        printf("%d  %s\n", i + 1, history[i].command);
    }
}

// Show detailed info (for exit or Ctrl+C — part h)
void show_detailed_history() {
    printf("\n--- Command Execution History ---\n");
    for (int i = 0; i < history_count; i++) {
        printf("[%d] PID: %d | Start: %s | Duration: %.0f sec | Command: %s\n",
            i + 1,
            history[i].pid,
            ctime(&history[i].start_time),
            history[i].duration,
            history[i].command);
    }
}

// Stopping program (Ctrl+C)
void ctrlc(int signal) {
    printf("\nExecution stopped using Ctrl+C\n");
    show_detailed_history();
    exit(0);
}

// Parse command string into arguments
void parse_args(char *command, char **argv) {
    char *token = strtok(command, " ");
    int i = 0;
    while (token != NULL && i < MAX_ARGS - 1) {
        argv[i++] = token;
        token = strtok(NULL, " ");
    }
    argv[i] = NULL;
}

// Execute a single command (no pipes)
int create_process_and_run(char *command) {
    char command_copy[MAX_INPUT];
    strncpy(command_copy, command, MAX_INPUT);

    parse_args(command, args);
    time_t start = time(NULL);
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 0;
    } else if (pid == 0) {//Child Process
        execvp(args[0], args);
        perror("execvp failed");//if cmd didn't run , error
        exit(1);
    } else {//Parent Process
        int wstatus;
        waitpid(pid, &wstatus, 0);//wait for child process to die

        time_t end = time(NULL);
        double duration = difftime(end, start);

        add_to_history(command_copy, pid, start, duration);
    }
    return 1;
}

// Execute multiple piped commands
int execute_pipes(char *commands[], int num_cmds) {
    int pipes[MAX_CMDS - 1][2]; // Each pipe connects two commands

    // Create all required pipes
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe failed");
            return 0;
        }
    }

    pid_t pids[MAX_CMDS];
    time_t start = time(NULL);

    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // If not first command, redirect stdin to previous pipe
            if (i > 0) {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            // If not last command, redirect stdout to next pipe
            if (i < num_cmds - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }
            // Close all pipe fds in child
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            char *argv[MAX_ARGS];
            parse_args(commands[i], argv);
            execvp(argv[0], argv);
            perror("execvp failed");
            exit(1);
        } else {
            pids[i] = pid;
        }
    }

    // Parent closes all pipes
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Parent waits for all children
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], NULL, 0);
    }

    time_t end = time(NULL);
    double duration = difftime(end, start);

    // Store as a single entry in history (combine whole pipeline)
    char combined[MAX_INPUT] = "";
    for (int i = 0; i < num_cmds; i++) {
        strcat(combined, commands[i]);
        if (i < num_cmds - 1) strcat(combined, " | ");
    }
    add_to_history(combined, pids[num_cmds - 1], start, duration);

    return 1;
}

// Launch command with support for multiple pipes
int launch(char *command) {
    if (strcmp(command, "history") == 0) {
        show_history_commands(); // part (g)
        return 1;
    }

    // Split by pipe symbols
    char *cmds[MAX_CMDS];
    int num_cmds = 0;

    char *token = strtok(command, "|");
    while (token != NULL && num_cmds < MAX_CMDS) {
        while (*token == ' ') token++; // Skip leading spaces
        cmds[num_cmds++] = token;
        token = strtok(NULL, "|");
    }

    if (num_cmds == 1) {
        return create_process_and_run(cmds[0]);
    } else {
        return execute_pipes(cmds, num_cmds);
    }
}

// Read input from user
char* read_user_input() {
    static char buffer[MAX_INPUT];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        printf("\n");
        exit(0);
    }
    buffer[strcspn(buffer, "\n")] = '\0';
    return buffer;
}

// Main shell loop
void shell_loop() {
    int status;
    do {
        printf("iiitd@possum:~$ ");
        fflush(stdout);
        char *command = read_user_input();

        if (strlen(command) == 0) continue;

        if (strcmp(command, "exit") == 0) {
            show_detailed_history(); // part (h)
            break;
        }

        status = launch(command);
    } while (status);
}

int main() {
    signal(SIGINT, ctrlc);
    shell_loop();
    return 0;
}
