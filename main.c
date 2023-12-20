#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define HISTORY_SIZE 10

void executeCommand(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("shell");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

void changeDirectory(char *directory) {
    if (directory == NULL) {
        fprintf(stderr, "shell: expected argument to \"cd\"\n");
    } else {
        if (chdir(directory) != 0) {
            perror("shell");
        }
    }
}

void exitShell() {
    printf("Exiting the shell.\n");
    exit(EXIT_SUCCESS);
}

void displayPrompt() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Shell [%s]> ", cwd);
    } else {
        perror("getcwd");
        printf("Shell> ");
    }
    fflush(stdout);
}

void addCommandToHistory(char *command, char **history, int *historyIndex, int *historyCount) {
    if (*historyCount < HISTORY_SIZE) {
        history[*historyCount] = strdup(command);
        (*historyCount)++;
    } else {
        free(history[*historyIndex]);
        history[*historyIndex] = strdup(command);
        *historyIndex = (*historyIndex + 1) % HISTORY_SIZE;
    }
}

void displayHistory(char **history, int historyCount) {
    printf("\nCommand History:\n");
    for (int i = 0; i < historyCount; ++i) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_INPUT_SIZE / 2 + 1];
    char *history[HISTORY_SIZE];
    int historyIndex = 0;
    int historyCount = 0;

    while (1) {
        displayPrompt();

        // Read user input
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';

        // Add the command to history
        addCommandToHistory(input, history, &historyIndex, &historyCount);

        // Parse input into arguments
        char *token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            args[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        args[i] = NULL;

        // Check for the "cd" command
        if (strcmp(args[0], "cd") == 0) {
            changeDirectory(args[1]);
            continue;
        }

        // Check for the "exit" command
        if (strcmp(args[0], "exit") == 0) {
            exitShell();
        }

        // Check for the "history" command
        if (strcmp(args[0], "history") == 0) {
            displayHistory(history, historyCount);
            continue;
        }

        // Execute the command
        executeCommand(args);
    }

    // Free memory allocated for history
    for (int i = 0; i < HISTORY_SIZE; ++i) {
        free(history[i]);
    }

    return 0;
}
