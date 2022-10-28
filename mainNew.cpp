#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <string>
#include <vector>

time_t startTime = 0;
std::vector<char*> dirHistory;
char* homeDir;

// get current time in milliseconds
void getStarttime() {
    startTime = (time(NULL) - startTime) * 1000;
}

// elapsedTime in milliseconds
int getElapsedTime() {
   return (time(NULL) *1000 - startTime);
}

char* getPrefixes() {
    char *pwd = getcwd(NULL, 0);
    // Check if we are in or below home directory
    if (strstr(pwd, homeDir) == pwd) {
	// We are in or below home directory
	char *prefix = (char*) malloc(strlen(pwd) - strlen(homeDir) + 2);
	std::string colorStart = "\033[1;32m";
	std::string colorEnd = "\033[0m";
	strcpy(prefix, colorStart.c_str());
	strcat(prefix, "~");
	strcat(prefix, pwd + strlen(homeDir));
	strcat(prefix, colorEnd.c_str());
	return prefix;
    } else {
	// We are outside home directory
	char *prefix = (char*) malloc(strlen(pwd) + 2);
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
    
    if ((pid = waitpid( -1 , &status, WNOHANG)) > 0) {
        printf("\nProcess %d exited\n", pid);

    }

}

// define function for changing the directory
int cd(char *path) {
    // Check for tilde
    if (path[0] == '~')
    {
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
    } else if (path[0] == '-')
    {
        // Check if there is a previous directory
        if (dirHistory.size() > 1)
        {
            // Get the previous directory
            char *prevDir = dirHistory[dirHistory.size() - 2];
            // Change the directory
            ::chdir(prevDir);
            // Remove the previous directory from the history
            dirHistory.pop_back();
            return 0; // Prevent the current directory from being added to the history
        }
    } else {

        if (::chdir(path) != 0)
        {
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
    char* delim = " \n";
    // get line from stdin
    getline(&line, &len, stdin);

    char *tmp = strtok(line, delim);

    int i = 0;

    // split line into parameters
    // allocate memory for parameters array
    *par = (char ** ) malloc(sizeof(char*) * 10);
    int background = 0;
    while (tmp != NULL) {
        
        // if last element is "&"
        if (strcmp(tmp, "&") == 0) {
            // set last element to NULL
            // set background flag
            (*par)[i] = NULL;
            background = 1;
            break;
        } else {

            (*par)[i] = tmp;
            i++;
            tmp = strtok(NULL, delim);
        }
    }

    *com = *par[0];
    
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
    getStarttime();

    signal(SIGINT, signalHandler);
    
    int childPid;
    int status;
    char *command;
    char **parameters;

    getElapsedTime();
    while (1) {

        int background = read_command(&command, &parameters);

        // if command is empty continue
        if (command == NULL) {
            continue;
        }
    
        if (strcmp(command, "exit") == 0) {
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

        if ((childPid = fork()) == -1) {
            fprintf(stderr,"can't fork\n");
            exit(1);
        } else if (childPid == 0) { /* child */

            execvp(command, parameters);
            // print error if execution failed
            perror(command);
                
            exit(0);

        // if last parameter is & then run in background
        } else if (background) {
            // if background flag is set
            // it is not possible to wait for child process
            // because it doesn't enters the wait() function
            // store command and parameters in array
            char* comm = command;
            printf("[%d]\n", childPid);
            signal(SIGCHLD, endSignalHandler);
        } else { /* parent process */
            // parent waits for child to finish
            wait(&status);
        } /* endif parent */
   } /* end while forever */
}

