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

// get current time in milliseconds
void setStarttime() { startTime = (time(NULL) - startTime) * 1000; }

// elapsedTime in milliseconds
int getElapsedTime() { return (time(NULL) * 1000 - startTime); }
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
/************************
function: void pipeCommand(char** cmd1, char** cmd2)
comment: This pipes the output of cmd1 into cmd2.
**************************/
/* void pipeCommand(char** cmd1, char** cmd2) { */
/*   int fds[2]; // file descriptors */
/*   pipe(fds); */
/*   // child process #1 */
/*   try{ */

/*   printf("cmd1: %s %s %s %s", cmd1[0], cmd1[1], cmd1[2], cmd1[3]); */
/*   printf("cmd2: %s %s %s %s", cmd2[0], cmd2[1], cmd2[2], cmd2[3]); */
/*   }  catch(int e){ */
/*     printf("Exception: %d", e); */
/*   } */
/*   if (fork() == 0) { */
/* 	  std::cout << "child 1" << std::endl; */
/*     // Reassign stdin to fds[0] end of pipe. */
/*     dup2(fds[0], STDIN_FILENO); */
/*     close(fds[1]); */
/*     close(fds[0]); */
/*     // Execute the second command. */
/*     // child process #2 */
/*     if (fork() == 0) { */
/* 	    		std::cout << "child 2" << std::endl; */
/*         // Reassign stdout to fds[1] end of pipe. */
/*         dup2(fds[1], STDOUT_FILENO); */
/*         close(fds[0]); */
/*         close(fds[1]); */
/*         // Execute the first command. */
/*         execvp(cmd1[0], cmd1); */
/*     } */
/*     wait(NULL); */
/*     printf("OUTPUT CMD 2"); */
/*     execvp(cmd2[0], cmd2); */
/*     } */
/*     close(fds[1]); */
/*     close(fds[0]); */
/*     wait(NULL); */
/* } */

char *getPrefixes() {
    char *pwd = getcwd(NULL, 0);
    // Check if we are in or below home directory
    if (strstr(pwd, homeDir) == pwd) {
        // We are in or below home directory
        char *prefix = (char *)malloc(strlen(pwd) - strlen(homeDir) + 2);
        std::string colorStart = "\033[1;32m";
        std::string colorEnd = "\033[0m";
        strcpy(prefix, colorStart.c_str());
        strcat(prefix, "~");
        strcat(prefix, pwd + strlen(homeDir));
        strcat(prefix, colorEnd.c_str());
        return prefix;
    } else {
        // We are outside home directory
        char *prefix = (char *)malloc(strlen(pwd) + 2);
        std::string colorStart = "\033[1;32m";
        std::string colorEnd = "\033[0m";
        strcpy(prefix, colorStart.c_str());
        strcat(prefix, pwd);
        strcat(prefix, colorEnd.c_str());
        return prefix;
    }
}

// define action performed by detecting signal
void signalHandler(int signum) {
    // Do nothing
    fprintf(stdout, "\n%s > ", getPrefixes());
    /* printf("\nTime: %dms\n", getElapsedTime()); */

    /* exit(signum); */
}

// define action performed by detecting signal
void endSignalHandler(int signum) {
    int status;
    pid_t pid;

    if ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("\nProcess %d exited\n", pid);
    }
}

// define function for changing the directory
int cd(char *path) {
    // Check for tilde
    if (path[0] == '~') {
        chdir(homeDir);
        // Get the home directory
        // Create a new path
        /* char *newPath = (char *)malloc(strlen(homeDir) + strlen(path)); */
        /* // Copy the home directory */
        /* strcpy(newPath, home); */
        /* // Copy the rest of the path */
        /* strcat(newPath, path + 1); */
        /* // Change the directory */
        /* chdir(newPath); */
        /* // Free the memory */
        /* free(newPath); */
    } else if (path[0] == '-') {
        // Check if there is a previous directory
        if (dirHistory.size() > 1) {
            // Get the previous directory
            char *prevDir = dirHistory[dirHistory.size() - 2];
            // Change the directory
            ::chdir(prevDir);
            // Remove the previous directory from the history
            dirHistory.pop_back();
            return 0;  // Prevent the current directory from being added to the
                       // history
        }
    } else {
        if (::chdir(path) != 0) {
            printf("Directory not found\n");
            return 1;
        }
        // Set the new path for history
    }
    // Add the new path to the history
    dirHistory.push_back(getcwd(NULL, 0));
    return 0;
}

/*
 * read command and parameters from stdin as call-by-reference
 * return if background command is detected
 */
int read_command(char **com, char ***par) {
    // display pwd in front of prompt
    /* char *pwd = getcwd(NULL, 0); */
    fprintf(stdout, "%s > ", getPrefixes());

    char *line = NULL;
    size_t len = 0;
    char *delim = " \n";
    // get line from stdin
    getline(&line, &len, stdin);

    char *tmp = strtok(line, delim);

    int i = 0;

    // split line into parameters
    // allocate memory for parameters array
    *par = (char **)malloc(sizeof(char *) * 10);
    int background =
        0;  // Replace with type: enum { FOREGROUND, BACKGROUND, PIPE }
    bool paramQuote = false;
    while (tmp != NULL) {
        // if last element is "&"
        if (strcmp(tmp, "&") == 0) {
            // set last element to NULL
            // set background flag
            (*par)[i] = NULL;
            background = 1;
            break;
        } else if (strcmp(tmp, "|") == 0) {
            // set last element to NULL
            // set background flag
            (*par)[i] = NULL;
            background = 2;
            char **cmd1 = *par;
            char **cmd2 = (char **)malloc(sizeof(char *) * 10);
            // Params and command for second command
            int j = 0;
            tmp = strtok(NULL, delim);
            while (tmp != NULL) {
                cmd2[j] = tmp;
                j++;
                tmp = strtok(NULL, delim);
            }
            cmd2[j] = NULL;
            printf("\nCommands for PIPE: 1. %s %s 2. %s %s \n\n", cmd1[0],
                   cmd1[1], cmd2[0], cmd2[1]);
            // Execute the commands
            pipe_cmd(cmd1, cmd2);
            /* pipeCommand(cmd1, cmd2); */
            break;
        } else {
            if (!paramQuote) {
                if (tmp[0] != '"') {
                    (*par)[i] = tmp;
                    i++;
                    tmp = strtok(NULL, delim);
                } else {
                    paramQuote = true;
                    (*par)[i] = tmp;
                    i++;
                    tmp = strtok(NULL, delim);
                }
            } else {
                if (tmp[strlen(tmp) - 1] != '"') {
                    strcat((*par)[i - 1], " ");
                    strcat((*par)[i - 1], tmp);
                    tmp = strtok(NULL, delim);
                } else {
                    strcat((*par)[i - 1], " ");
                    strcat((*par)[i - 1], tmp);
                    paramQuote = false;
                    i++;
                    tmp = strtok(NULL, delim);
                }
            }
        }
    }
    *com = *par[0];
    // Go through the parameters and put the param in the double quotes in one
    // string
    /* for (int j = 0; j < i; j++) { */
    /* if ((*par)[j][0] == '"') { */
    /* // We have a double quote */
    /* // Get the length of the string */
    /* int len = strlen((*par)[j]); */
    /* // Check if the last character is a double quote */
    /* if ((*par)[j][len - 1] == '"') { */
    /* // We have a double quote */
    /* // Remove the double quotes */
    /* (*par)[j][len - 1] = '\0'; */
    /* (*par)[j] = (*par)[j] + 1; */
    /* } else { */
    /* // We don't have a double quote */
    /* // Get the next parameter */
    /* j++; */
    /* while (j < i) { */
    /* // Get the length of the string */
    /* int len = strlen((*par)[j]); */
    /* // Check if the last character is a double quote */
    /* if ((*par)[j][len - 1] == '"') { */
    /* // We have a double quote */
    /* // Remove the double quotes */
    /* (*par)[j][len - 1] = '\0'; */
    /* break; */
    /* } else { */
    /* // We don't have a double quote */
    /* // Get the next parameter */
    /* j++; */
    /* } */
    /* } */
    /* } */
    /* } */
    /* } */
    // Replace the double quotes in the parameters with single quotes
    /* for (int j = 0; j < i; j++) { */
    /* char *param = (*par)[j]; */
    /* for (int k = 0; k < strlen(param); k++) { */
    /* if (param[k] == '"') { */
    /* param[k] = '\''; */
    /* } */
    /* } */
    /* } */
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
    dirHistory.push_back(getcwd(NULL, 0));
    homeDir = std::getenv("HOME");
    setStarttime();

    signal(SIGINT, signalHandler);

    int childPid;
    int status;
    char *command;
    char **parameters;

    // Test Pipe
    /* printf("TEST PIPE\n"); */
    /* char **cmd1 = (char **) malloc(sizeof(char*) * 10); */
    /* char **cmd2 = (char **) malloc(sizeof(char*) * 10); */
    /* cmd1[0] = "env"; */
    /* cmd1[1] = NULL; */
    /* cmd2[0] = "grep"; */
    /* cmd2[1] = "PATH"; */
    /* printf("cmd1: %s params %s", cmd1[0], cmd1[1]); */
    /* pipe_cmd(cmd1, cmd2); */
    /* pipeCommand(cmd1, cmd2); */
    /* printf("Test pipe done\n"); */

    getElapsedTime();
    while (1) {
        int background = read_command(&command, &parameters);

        // if command is empty continue
        if (command == NULL ||
            background == 2) {  // 2 is for pipe => handled in read_command
                                /* printf("PIPE OR NULL"); */
            continue;
        }
        if (strcmp(command, ":q") == 0) {
            return 0;
        }
        if (strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) {
            // ask if user wants to exit
            printf("Do you want to quit? (y/n) ");
            char answer;
            scanf("%c", &answer);
            if (answer == 'y') {
                printf("Time: %dms\n", getElapsedTime());
                printf("Goodbye!\n");
                return 0;
            }
        } else if (strcmp(command, "cd") == 0) {
            // change directory
            if (cd(parameters[1]) < 0) {
                // return error message if directory doesn't exist
                perror(parameters[1]);
            }

            continue;
        }
        printf("Command: %s\n", command);
        printf("Parameters: %s %s %s %s %s %s %s %s %s %s\n", parameters[0],
               parameters[1], parameters[2], parameters[3], parameters[4],
               parameters[5], parameters[6], parameters[7], parameters[8],
               parameters[9]);
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
            // if background flag is set
            // it is not possible to wait for child process
            // because it doesn't enters the wait() function
            // store command and parameters in array
            char *comm = command;
            printf("[%d]\n", childPid);
            signal(SIGCHLD, endSignalHandler);
        } else if (background == 0) { /* parent process */
            // parent waits for child to finish
            wait(&status);
        } /* endif parent */
    }     /* end while forever */
}
