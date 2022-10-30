#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>
time_t startTime = 0;
std::vector<char *> dirHistory;
char *homeDir;

// get current time in seconds
void setStarttime() { startTime = time(NULL); }

// elapsedTime in seconds
int getElapsedTime() { return time(NULL) - startTime; }

// This pipes the output of cmd1 into cmd2.
void pipe_cmd(char **cmd1, char **cmd2) {
    int fds[2];  // file descriptors
    pipe(fds);
    pid_t pid;

    // child process #1
    if (fork() == 0) {
        // Reassign stdin to fds[0] end of pipe.
        dup2(fds[0], 0);

        // Not going to write in this child process, so we can close this end
        // of the pipe.
        close(fds[1]);

        // Execute the second command.
        execvp(cmd2[0], cmd2);
        perror("execvp failed");

        // child process #2
    } else if ((pid = fork()) == 0) {
        // Reassign stdout to fds[1] end of pipe.
        dup2(fds[1], 1);

        // Not going to read in this child process, so we can close this end
        // of the pipe.
        close(fds[0]);

        // Execute the first command.
        execvp(cmd1[0], cmd1);
        perror("execvp failed");

        // parent process
    } else
        waitpid(pid, NULL, 0);
}

char *getPrefixes() {
    char *pwd = getcwd(NULL, 0);
    if (homeDir == NULL) {
        homeDir = getenv("HOME");
        // If homeDir is NULL, then the user doesn't have a home directory.
        if (homeDir == NULL) {
            return pwd;
        }
    }
    const std::string colorStart = "\033[1;32m";
    const std::string colorEnd = "\033[0m";
    // Check if we are in or below home directory
    if (strstr(pwd, homeDir) == pwd) {
        // We are in or below home directory
        char *prefix = (char *)malloc(strlen(pwd) - strlen(homeDir) + 2);

        strcpy(prefix, colorStart.c_str());
        strcat(prefix, "~");
        strcat(prefix, pwd + strlen(homeDir));
        strcat(prefix, colorEnd.c_str());
        return prefix;
    } else {
        // We are outside home directory
        char *prefix = (char *)malloc(strlen(pwd) + 2);

        strcpy(prefix, colorStart.c_str());
        strcat(prefix, pwd);
        strcat(prefix, colorEnd.c_str());
        return prefix;
    }
}

/*
 * Define action performed by detecting signal
 * This function is called when the user presses Ctrl+C.
 */
void signalHandler(int signum) {
    // Do nothing
    printf("\n%s > ", getPrefixes());  // TODO: print directly
    // exit(signum);
}

// All childPids for cleanup
std::vector<int> childPids;

/*
 * Define action performed by detecting signal
 * This function is called when the a background process finishes.
 */
void endSignalHandler(int signum) {
    int status;
    pid_t pid;

    if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("\nProcess %d exited\n", pid);
        childPids.erase(std::remove(childPids.begin(), childPids.end(), pid),
                        childPids.end());
    }
}

int cdToHome() {
    if (homeDir == NULL) {
        homeDir = getenv("HOME");
    }
    if (homeDir == NULL) {
        printf("No home directory found.\n");
        return -1;
    }
    chdir(homeDir);
    dirHistory.push_back(homeDir);
    return 0;
}

// define function for changing the directory
int cd(char *path) {
    if (path == NULL) {
        return cdToHome();
    } else {
        // Check for tilde
        if (path[0] == '~') {
            return cdToHome();
        } else if (path[0] == '-') {
            // Check if there is a previous directory
            if (dirHistory.size() > 1) {
                // Get the previous directory
                char *prevDir = dirHistory[dirHistory.size() - 2];
                // Change the directory
                ::chdir(prevDir);
                delete[] prevDir;
                // Remove the previous directory from the history
                dirHistory.pop_back();
                // Early return
                return 0;  // Prevent the current directory from being added to
                           // the history
            } else {
                printf("No previous directory found.\n");
                return -1;
            }
        } else {
            if (::chdir(path) != 0) {
                printf("Directory not found\n");
                return -11;
            }
        }
    }
    // Add the new path to the history
    dirHistory.push_back(getcwd(NULL, 0));
    return 0;
}

void pipeCommand(char **cmd1Args, char **cmd2Args) {
    // Create pipe
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid1, pid2;
    pid1 = fork();
    if (pid1 == 0) {
        // Child 1
        // Close read end of pipe
        close(pipefd[0]);
        // Redirect stdout to write end of pipe
        dup2(pipefd[1], STDOUT_FILENO);
        // Close write end of pipe
        close(pipefd[1]);
        // Execute command
        execvp(cmd1Args[0], cmd1Args);
        // If execvp returns, there was an error
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent
        pid2 = fork();
        if (pid2 == 0) {
            // Child 2
            // Close write end of pipe
            close(pipefd[1]);
            // Redirect stdin to read end of pipe
            dup2(pipefd[0], STDIN_FILENO);
            // Close read end of pipe
            close(pipefd[0]);
            // Execute command
            execvp(cmd2Args[0], cmd2Args);
            // If execvp returns, there was an error
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            // Parent
            // Close both ends of pipe
            close(pipefd[0]);
            close(pipefd[1]);
            // Wait for children to finish
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
    }
}

/*
 * read command and parameters
 * return if background command is detected
 */
int read_command(char **com, char ***par) {
    // display prefix
    fprintf(stdout, "%s > ", getPrefixes());

    char *line = NULL;
    size_t len = 0;
    char *delim = "\n";
    // getch
    int c;
    *par = (char **)malloc(sizeof(char *) * 10);
    bool isAPipe = false;
    int pipeIndex = 0;

    while ((c = getchar()) != EOF && c != '\n') {
        line = (char *)realloc(line, len + 1);

        line[len++] = (char)c;
        if (c == '|') {
            isAPipe = true;
            pipeIndex = len - 1;
        }
    }
    // Handle the PIPE
    if (isAPipe) {
        char *cmd1 = (char *)malloc(pipeIndex + 1);
        char *cmd2 = (char *)malloc(len - pipeIndex);
        strncpy(cmd1, line, pipeIndex);
        cmd1[pipeIndex] = '\0';
        strncpy(cmd2, line + pipeIndex + 1, len - pipeIndex - 1);
        cmd2[len - pipeIndex - 1] = '\0';
        char **cmd1Args = (char **)malloc(sizeof(char *) * 10);
        char **cmd2Args = (char **)malloc(sizeof(char *) * 10);
        int cmd1Argc = 0;
        int cmd2Argc = 0;
        char *token = strtok(cmd1, " ");
        while (token != NULL) {
            cmd1Args[cmd1Argc++] = token;
            token = strtok(NULL, " ");
        }
        cmd1Args[cmd1Argc] = NULL;
        token = strtok(cmd2, " ");
        while (token != NULL) {
            cmd2Args[cmd2Argc++] = token;
            token = strtok(NULL, " ");
        }
        cmd2Args[cmd2Argc] = NULL;
        pipeCommand(cmd1Args, cmd2Args);

        return 2;
    }

    // Check if line is empty
    if (line == NULL) {
        return -1;
    }

    int background = 0;
    // Check if last character is &
    if (line[len - 1] == '&') {
        background = 1;
        line[len - 1] = '\0';
    }
    // Split the line into parameters
    // TODO: handle quotes
    char *token = strtok(line, " ");
    int argc = 0;
    // Loop through the tokens and put everything in quotes into one parameter
    bool inQuotes = false;
    char *param = NULL;
    while (token != NULL) {     // Token = each word
        if (token[0] == '"') {  // Start of token is a quote
            inQuotes = true;    // Set inQuotes to true
            param = (char *)malloc(strlen(token));
            strcpy(param, token + 1);  // Copy the token without the first quote
        } else if (token[strlen(token) - 1] == '"') {  // Current element is the
                                                       // end of a parameter
                                                       // with spaces
            inQuotes = false;                          // Set inQuotes to false
            strcat(param, " ");                        // Add a space
            strcat(param, token);                      // Add the token
            param[strlen(param) - 1] = '\0';           // Remove the last quote
            (*par)[argc++] = param;  // Add the parameter to the list
        } else if (inQuotes) {   // Current element is part of a parameter with
                                 // spaces
            strcat(param, " ");  // Add a space
            strcat(param, token);  // Add the token
        } else {
            (*par)[argc++] = token;  // Add the token to the list
        }
        token = strtok(NULL, " ");  // Get the next token
    }
    (*par)[argc] = NULL;  // Add NULL to the end of the list
    // Set the command
    *com = (*par)[0];
    return background;
}

/*
 * fork() creates new child process
 */

/*
 * execv() executes a command with null terminated parameters
 */

/*
 * wait() waits for child process to terminate
 */

/*
 * zombie processes have finished their execution
 * but are still in the process table
 */

int main() {
    dirHistory.push_back(
        getcwd(NULL, 0));           // Add the current directory to the history
    homeDir = std::getenv("HOME");  // Get the home directory
    setStarttime();                 // Set the start time

    signal(SIGINT, signalHandler);  // Register signal handler for Ctrl+C

    int childPid;
    int status;
    char *command;
    char **parameters;

    while (1) {
        int background = read_command(&command, &parameters);

        // if command is empty continue
        if (command == NULL || background == 2 ||
            background == -1) {  // 2 is for pipe => handled in read_command
            continue;
        }
        if (strcmp(command, ":q") == 0) {  // Directly exit
            return 0;
        }
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            // ask if user wants to exit
            printf("Do you want to quit? (y/n) ");
            char answer;
            scanf("%c", &answer);
            if (answer == 'y') {
                printf("Time: %ds\n", getElapsedTime());
                // Kill all child processes before exiting
                for (int i = 0; i < childPids.size(); i++) {
                    kill(childPids[i], SIGKILL);
                }
                printf("Goodbye!\n");
                return 0;
            }
        } else if (strcmp(command, "cd") == 0) {  // change directory
            if (cd(parameters[1]) < 0) {
                // return error message if directory doesn't exist
                perror(parameters[1]);
            }

            continue;
        }
        if ((childPid = fork()) == -1) {
            fprintf(stderr, "can't fork\n");
            exit(1);
        } else if (childPid == 0) { /* child */

            execvp(command, parameters);
            // print error if execution failed
            perror(command);

            exit(0);

            // if last parameter is & then run in background
        } else if (background == 1) {
            childPids.push_back(childPid);
            char *comm = command;
            printf("[%d]\n", childPid);
            signal(SIGCHLD, endSignalHandler);
        } else if (background == 0) { /* parent process */
            // parent waits for child to finish
            wait(&status);
        }
    }
}
