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
            printf("\r\n");
            perror("shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        printf("\r\n");
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
        fprintf(stderr, "\r\nshell: expected argument to \"cd\"\n");
    } else {
        if (chdir(directory) != 0) {
            perror("shell");
        }
        printf("\r\n");
    }
}

void exitShell() {
    printf("\r\nExiting the shell.\n");
    printf("\r");

    exit(EXIT_SUCCESS);
    
    sleep(1);
    endwin();
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
    printf("\r\nCommand History:\n");
    for (int i = 0; i < historyCount; ++i) {
        printf("\r%d: %s\n", i + 1, history[i]);
    }
}

void handleInput(char *input, int *index, char **history, int *historyIndex, int *historyCount, char *cwd) {
    int ch;
    while ((ch = getch()) != '\n') {
        switch (ch) {
            case KEY_UP:
                // Up arrow key pressed, cycle through history
                if (*historyCount > 0) {
                    if (*historyIndex > 0) {
                        (*historyIndex)--;
                        printf("\rShell [%s]> ", cwd);
                        printf("\033[K%s", history[*historyIndex]);
                        fflush(stdout);
                        *index = strlen(history[*historyIndex]);
                        strcpy(input, history[*historyIndex]);
                    }
                    else
                    {
                        printf("\rShell [%s]> ", cwd);
                        printf("\033[K%s", history[*historyCount-1]);
                        fflush(stdout);
                        *index = strlen(history[*historyCount-1]);
                        strcpy(input, history[*historyCount-1]);
                        (*historyIndex)=(*historyCount)-1;
                    }
                }
                break;

            case KEY_DOWN:
                // Down arrow key pressed, cycle through history
                if (*historyCount > 0) {
                    if (*historyIndex < *historyCount - 1) {
                        (*historyIndex)++;
                        printf("\rShell [%s]> ", cwd);
                        printf("\033[K%s", history[*historyIndex]);
                        fflush(stdout);
                        *index = strlen(history[*historyIndex]);
                        strcpy(input, history[*historyIndex]);
                    }
                }
                break;

            case KEY_LEFT:
                // Left arrow key pressed, move cursor left
                if (*index > 0) {
                    printf("\033[D");  // Move left
                    fflush(stdout);
                    (*index)--;
                }
                break;

            case KEY_RIGHT:
                // Right arrow key pressed, move cursor right
                if (*index < strlen(input)) {
                    printf("\033[C");  // Move right
                    fflush(stdout);
                    (*index)++;
                }
                break;

            case KEY_BACKSPACE:
            // Backspace key pressed, handle deletion
                if (*index > 0) {
                    // Verifică dacă cursorul nu este la începutul inputului
                    if (*index < strlen(input)) {
                        // Șterge caracterul curent
                        for (int i = *index; i < strlen(input); ++i) {
                            input[i - 1] = input[i];
                        }
                        input[strlen(input) - 1] = '\0';
                
                        // Afisează întregul input actualizat
                        printf("\rShell [%s]> %s \033[K", cwd, input);

                        //scad indexul in stg pentru a muta cursorul
                        (*index)--;
                        
                        // Ramane cursorul acolo unde sterg rezolva un bug, de ce??? NU STIU dar il rezolva
                        for (int i = strlen(input); i >= *index; --i) {
                            printf("\b");
                        }
                        fflush(stdout);
                    } else {
                        // Muta cursorul la stânga și șterge caracterul
                        printf("\b \b");
                        fflush(stdout);
                        (*index)--;
                        input[*index] = '\0';
                    }
                }
                break;

            default:
                // Other keys
                if (*index < MAX_INPUT_SIZE - 1) {
                    // Deplasează caracterele la dreapta pentru a face loc noului caracter
                    for (int i = strlen(input); i > *index && strlen(input)>0; --i) {
                        input[i] = input[i - 1];
                    }

                    // Inserează noul caracter la poziția curenta a cursorului
                    input[*index] = ch;

                    // Afiseaza intregul input actualizat
                    printf("\rShell [%s]> %s \033[K", cwd, input);

                    // Muta cursorul la dreapta pentru a reflecta noul caracter
                    for (int i = *index; i < strlen(input); ++i) {
                        printf("\b");
                    }

                    (*index)++;

                    input[strlen(input)] = '\0';
                    
                    fflush(stdout);
                }
                
                break;

        }
    }
    
}

int main() {
    char *history[HISTORY_SIZE];
    int historyIndex = 0;
    int historyCount = 0;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    refresh();
    while (1) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd");
            return EXIT_FAILURE;
        }

        displayPrompt(cwd);

        char input[MAX_INPUT_SIZE];
        int index = 0;

        //resetez inputul
        for (int i=0;i<MAX_INPUT_SIZE;i++)
            input[i]='\0';

        // Read user input
        handleInput(input, &index, history, &historyIndex, &historyCount, cwd);

        // Execute the command only if Enter is pressed
        if (input[0] != '\0') {
            // Add the command to history
            addCommandToHistory(input, history, &historyIndex, &historyCount);
            index=strlen(input);
            // Parse input into arguments
            char *token = strtok(input, " ");
            int i = 0;
            char *args[MAX_INPUT_SIZE / 2 + 1];
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
        else
            continue;
    }

    endwin();
    
    // Free memory allocated for history
    for (int i = 0; i < HISTORY_SIZE; ++i) {
        free(history[i]);
    }

    return 0;
}
