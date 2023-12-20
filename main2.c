#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ncurses.h>

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

void displayPrompt(char *cwd) {
    printf("\rShell [%s]> ", cwd);
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

void handleArrowKeys(int key, char *input, int *historyIndex, char **history, int *historyCount) {
    if (key == KEY_UP) {
        // Up arrow key pressed, cycle through history
        if (*historyIndex > 0) {
            (*historyIndex)--;
            strcpy(input, history[*historyIndex]);
        }
    } else if (key == KEY_DOWN) {
        // Down arrow key pressed, cycle through history
        if (*historyIndex < *historyCount - 1) {
            (*historyIndex)++;
            strcpy(input, history[*historyIndex]);
        }
    } else if (key == KEY_BACKSPACE) {
        // Backspace key pressed, handle deletion
        int len = strlen(input);
        if (len > 0) {
            input[len - 1] = '\0';
            printf("\b \b");  // Move cursor back and clear the character
            fflush(stdout);
        }
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_INPUT_SIZE / 2 + 1];
    char *history[HISTORY_SIZE];
    int historyIndex = 0;
    int historyCount = 0;

    // Initialize ncurses
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    while (1) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd");
            return EXIT_FAILURE;
        }

        displayPrompt(cwd);

        // Read user input
        int ch;
        int i = 0;
        while ((ch = getch()) != '\n') {
            if (ch == KEY_UP || ch == KEY_DOWN || ch == KEY_BACKSPACE) {
                handleArrowKeys(ch, input, &historyIndex, history, &historyCount);
                continue;
            }

            if (i < MAX_INPUT_SIZE - 1) {
                input[i++] = ch;
                printf("%c", ch);
                fflush(stdout);
            }
        }
        input[i] = '\0';

        // Add the command to history
        addCommandToHistory(input, history, &historyIndex, &historyCount);

        // Parse input into arguments
        char *token = strtok(input, " ");
        i = 0;
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

        printf("\n");  // Move to a new line after executing the command
    }

    // End ncurses
    endwin();

    // Free memory allocated for history
    for (int i = 0; i < HISTORY_SIZE; ++i) {
        free(history[i]);
    }

    return 0;
}
