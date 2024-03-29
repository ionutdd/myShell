//Production

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define HISTORY_SIZE 10

pid_t pid;

//Afisare Prompt
void displayPrompt() 
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Shell [%s]> ", cwd);
    } else {
        perror("getcwd");
        printf("Shell> ");
    }
    fflush(stdout);
}

//Adaugarea comenzii la istoric
void addCommandToHistory(char *command, char **history, int *historyIndex, int *historyCount) 
{
    if (*historyCount < HISTORY_SIZE) {
        history[*historyCount] = strdup(command);
        (*historyCount)++;
    } else {
        free(history[*historyIndex]);
        history[*historyIndex] = strdup(command);
        *historyIndex = (*historyIndex + 1) % HISTORY_SIZE;
    }
}

//Schimbarea folderului
void changeDirectory(char *directory) 
{
    if (directory == NULL) { 
        //fprintf(stderr, "shell: expected argument to \"cd\"\n");
    }
    else {
        if (chdir(directory) != 0) 
            perror("shell"); 
    }
}

//Listarea folderului
void executeLs() 
{
    char *ls_args[] = {"ls", NULL};

    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        // Child process
        if (execvp(ls_args[0], ls_args) == -1) {
            perror("shell");
            exit(EXIT_FAILURE);
        }
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, &status, 0);
    } else {
        // Fork failed
        perror("fork");
    }
}

//Iesirea din shell
void exitShell() 
{
    printf("Exiting the shell.\n");
    exit(EXIT_SUCCESS);
}

//Exit in caz de folosirea CTRL+Z
void sigtstp_handler(int signo) 
{
    if (signo == SIGTSTP) {
        printf("\n");
        exitShell();
    }
}

//Afisarea istoricului
void displayHistory(char **history, int historyCount) 
{
    printf("\nCommand History:\n");
    for (int i = 0; i < historyCount; ++i) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

//Sterge istoricul 
void clearHistory(char **history, int *historyCount) {
    
    for (int i = 0; i < HISTORY_SIZE; ++i) {
        free(history[i]);
        history[i] = NULL;
    }
    *historyCount = 0;

    printf("Commands in history cleared!\n");
}

//Afiseaza numarul de comenzi din history
void displayHistoryCount(int historyCount) {
    printf("Number of commands in history: %d\n", historyCount);
}

//Afiseaza ultimele n comenzi
void displayRecentCommands(char **history, int historyCount, int number) {
    int start = (historyCount >= number) ? (historyCount - number) : 0;
    printf("\nRecent Commands:\n");
    for (int i = start; i < historyCount; ++i) {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

//Verificare daca un char este numar
int isNumber(const char *str) {
    char *endptr;
    strtol(str, &endptr, 10); // Attempt to convert the string to a long

    // Check if the conversion was successful and the entire string was consumed
    return (*str != '\0' && *endptr == '\0');
}

//Executarea ultimei linii din history
void executeLastCommand(char **history, int *historyIndex, int historyCount) {
    if (historyCount == 1) {
        printf("No commands in history.\n");
        return;
    }

    char *lastCommand = history[historyCount-2];
    printf("Executing last command: %s\n", lastCommand);


    // CD 
    if (strcmp(lastCommand, "cd") == 0) {
        changeDirectory(lastCommand);
        
    }
    else
    // LS
    if (strcmp(lastCommand, "ls") == 0 ) {
        executeLs();
        
    }
    else
    // EXIT
    if (strcmp(lastCommand, "exit") == 0 ) {
        exitShell();
    }
    else
    // EXIT cu ^Z
    if (strcmp(lastCommand, "^Z") == 0) {
        exitShell();
    }
    else
    // HISTORY-C
    if (strcmp(lastCommand, "history-c") == 0 && historyCount > 0 ) {
        clearHistory(history, &historyCount);
        
    }
    else    
    // HISTORY-N
    if (strcmp(lastCommand, "history-n") == 0 ){
        displayHistoryCount(historyCount);
        
    }
    else
    // HISTORY-NUMBER
    if (strstr(lastCommand, "history-") != NULL ){
    
        char *numberPart = lastCommand + 8; 
        if (isNumber(numberPart)) {
            int number = atoi(numberPart);
            displayRecentCommands(history, historyCount, number);
            
        }
        else
        {
            printf("Wrong command!\n");
            
        }

    }
    else
    // HISTORY
    if (strcmp(lastCommand, "history") == 0 ){
        displayHistory(history, historyCount);
        
    }
    else
        printf("Wrong command!\n");
    
}

//Clear la Shell
void clearShell() {
    // Pentru Windows
    #ifdef _WIN32
        system("cls");
    //pentru Linux/Unix
    #else
        system("clear");
    #endif
}


//Pipe


int evaluate_command(const char* command) {
    // Tokenize the command into arguments
    char* args[1024];
    char* token = strtok((char*)command, " ");
    int i = 0;
    while (token != NULL) {
        args[i] = token;
        token = strtok(NULL, " ");
        i++;
    }
    args[i] = NULL;

    // Create a child process to execute the command
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execvp(args[0], args);
        // If execvp fails
        perror("execvp");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status); // Return the exit status of the child process
    } else {
        // Fork failed
        perror("fork");
        return EXIT_FAILURE;
    }
}

int operator_pipe(const char* a_left, const char* a_right) {
    pid_t son = fork();
    if (son < 0) {
        perror("Error at fork");
        return EXIT_FAILURE;
    }

    if (son == 0) {
        // The child will do the piping operator

        // We create an array with two file descriptors and then call the
        // pipe system call to assign those file descriptors accordingly
        // fd[0] - read
        // fd[1] - write
        int fd[2];

        if (pipe(fd) < 0) {
            perror("Error creating pipe");
            exit(EXIT_FAILURE);
        }

        pid_t grandson = fork(); // The child creates another child that will execute the left command
        if (grandson < 0) {
            perror("Error at second fork");
            exit(EXIT_FAILURE);
        }

        if (grandson == 0) {
            // Grandson code for the left command

            // Close the read end because we will not use it
            close(fd[0]);

            // Redirect stdout to the write end of the pipe
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);

            // Execute the left command
            int left_status = evaluate_command(a_left);

            // We stop the grandson with the exit value of the left command
            exit(left_status);
        }

        pid = grandson;

        // Child code for the right command

        // Close the write end because we will not use it
        close(fd[1]);

        // Redirect stdin to the read end of the pipe
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        // Wait for the left command to finish then get the
        // status of the left command
        int status;
        waitpid(grandson, &status, WUNTRACED);
        int left_command_status = WEXITSTATUS(status);

        // If the left command did not execute successfully
        if (left_command_status != EXIT_SUCCESS) {
            // Exit with the exit status of the left command
            exit(left_command_status);
        }

        // Execute the right command
        int right_status = evaluate_command(a_right);

        // Exit with the exit status of the right command
        exit(right_status);
    }

    // Wait for the right command to finish then get the status
    // of the right command
    int status;
    waitpid(son, &status, WUNTRACED);
    int overall_status = WEXITSTATUS(status);

    // Return the exit code of the right command
    return overall_status;
}




//operatorii logici and si or
//operatorii logici and si or
int operator_and(const char* a_left, const char* a_right)
{
    // The first command executed successfully
    if (evaluate_command(a_left) == 1)
    {
        // So we run the second one too
        return evaluate_command(a_right);
    }
    // The first command failed
    return 0;
}

int operator_or(const char* a_left, const char* a_right)
{
    // The first command failed
    if (evaluate_command(a_left) == 0)
    {
        // So we run the second one too
        return evaluate_command(a_right);
    }
    // The first command executed successfully
    return 1;
}


//suspendarea unui program 
void suspend_handler() 
{
    extern pid_t pid;

    
    printf("\n^Z\nStopped\n");
    kill(pid, SIGKILL);
    
}


//rularea unui program
void executeRunCommand(char *filename) {
    char *run_args[] = {"gcc", filename, "-o", "temp_executable", NULL};

    pid_t compile_pid;
    int compile_status;

    compile_pid = fork();
    if (compile_pid == 0) {
        // Child process for compilation
        if (execvp(run_args[0], run_args) == -1) {
            perror("shell");
            exit(EXIT_FAILURE);
        }
    } else if (compile_pid > 0) {
        // Parent process waiting for compilation
        waitpid(compile_pid, &compile_status, 0);

        if (WIFEXITED(compile_status) && WEXITSTATUS(compile_status) == 0) {
            // Compilation successful, proceed to execute
            char *execute_args[] = {"./temp_executable", NULL};

            pid_t execute_pid;
            int execute_status;

            execute_pid = fork();
            if (execute_pid == 0) {
                // Child process for execution
                if (execvp(execute_args[0], execute_args) == -1) {
                    perror("shell");
                    exit(EXIT_FAILURE);
                }
            } else if (execute_pid > 0) {
                // Parent process waiting for execution
                waitpid(execute_pid, &execute_status, 0);
            } else {
                // Fork failed
                perror("fork");
            }
        } else {
            // Compilation failed
            printf("Compilation failed. Unable to run the program.\n");
        }
    } else {
        // Fork failed
        perror("fork");
    }
}

//SHELL MAIN
int main() {

    signal(SIGQUIT, sigtstp_handler);

    char input[MAX_INPUT_SIZE];
    char *args[MAX_INPUT_SIZE / 2 + 1];
    char *history[HISTORY_SIZE];
    int historyIndex = 0;
    int historyCount = 0;

    while (1) 
    {
        //Afisarea Promptului
        displayPrompt();

        // Citim input-ul
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';

        // Adaugam comanda la history
        addCommandToHistory(input, history, &historyIndex, &historyCount);

        // Check for pipe command
        if (strstr(input, "|") != NULL) 
        {
            // Split the input into left and right commands
            char *left_cmd = strtok(input, "|");
            char *right_cmd = strtok(NULL, "|");

            // Trim leading and trailing whitespaces
            left_cmd = strtok(left_cmd, " \t");
            right_cmd = strtok(right_cmd, " \t");

            // Execute the pipe command
            operator_pipe(left_cmd, right_cmd);
        }
        // Check for AND command
        else if (strstr(input, "&&") != NULL) 
        {
            // Split the input into left and right commands
            char *left_cmd = strtok(input, "&&");
            char *right_cmd = strtok(NULL, "&&");

            // Trim leading and trailing whitespaces
            left_cmd = strtok(left_cmd, " \t");
            right_cmd = strtok(right_cmd, " \t");

            // Execute the AND command
            if (operator_and(left_cmd, right_cmd) == 0) {
                continue;  // Do not execute the right command if the left one fails
            }
        }
        // Check for OR command
        else if (strstr(input, "||") != NULL) 
        {
            // Split the input into left and right commands
            char *left_cmd = strtok(input, "||");
            char *right_cmd = strtok(NULL, "||");

            // Trim leading and trailing whitespaces
            left_cmd = strtok(left_cmd, " \t");
            right_cmd = strtok(right_cmd, " \t");

            // Execute the OR command
            if (operator_or(left_cmd, right_cmd) == 1) {
                continue;  // Do not execute the right command if the left one succeeds
            }
        }
        else
        {

            // Transformam input-ul in argumente
            char *token = strtok(input, " ");
            int i = 0;
            while (token != NULL) {
                args[i] = token;
                token = strtok(NULL, " ");
                i++;
            }
            args[i] = NULL;

            if (strcmp(args[0], "run") == 0 && args[1] != NULL) {
            executeRunCommand(args[1]);
            continue;
            }

            if (strcmp(args[0], "suspend") == 0) {
                suspend_handler();
                continue;
            }
            // CD 
            if (strcmp(args[0], "cd") == 0) {
                changeDirectory(args[1]);
                continue;
            }
            // LS
            if (strcmp(args[0], "ls") == 0 && args[1]==NULL) {
                executeLs();
                continue;
            }
            // EXIT
            if (strcmp(args[0], "exit") == 0 && args[1]==NULL) {
                exitShell();
            }
            // EXIT cu ^Z
            if (strcmp(args[0], "^Z") == 0) {
                exitShell();
            }
            // HISTORY-C
            if (strcmp(args[0], "history-c") == 0 && historyCount > 0 && args[1]==NULL) {
                clearHistory(history, &historyCount);
                continue;
            }    
            // HISTORY-N
            if (strcmp(args[0], "history-n") == 0 && args[1]==NULL){
                displayHistoryCount(historyCount);
                continue;
            }
            // HISTORY-NUMBER
            if (strstr(args[0], "history-") != NULL && args[1]==NULL){
            
                char *numberPart = args[0] + 8; 
                if (isNumber(numberPart)) {
                    int number = atoi(numberPart);
                    displayRecentCommands(history, historyCount, number);
                    continue;
                }
                else
                {
                    printf("Wrong command!\n");
                    continue;
                }

            }
            // HISTORY
            if (strcmp(args[0], "history") == 0 && args[1]==NULL){
                displayHistory(history,historyCount);
                continue;
            }
            // !!
            if (strcmp(args[0], "!!") == 0 && args[1]==NULL) {
                executeLastCommand(history, &historyIndex, historyCount);
                continue;
            }
            // CLEAR
            if (strcmp(args[0], "clear") == 0 && args[1]==NULL) {
                clearShell();
                continue;
            }
            printf("Wrong command!\n");
        }
    }

    // Dealocam memoria care a fost alocata pentru history
    for (int i = 0; i < HISTORY_SIZE; ++i) {
        free(history[i]);
    }

    return 0;
}
