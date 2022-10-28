#include <csignal>
#include <iostream>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
std::vector<char*> dirHistory;



int getRuntime() {

    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = endTime - start;
    return elapsed_seconds.count();
}


void sig_handler(int s) {
// Do nothing
}
enum ownCommands  { help, cd, EXIT};

ownCommands getOwnCommand(std::string command)
{
    if (command == "help")
        return help;
    else if (command == "cd")
        return cd;
    else if (command == "exit")
        return EXIT;
    else
        return (ownCommands) -1;
};

void changeDir(const char *path)
{
    // Check for tilde
    if (path[0] == '~')
    {
        // Get the home directory
        char *home = getenv("HOME");
        // Create a new path
        char *newPath = (char *)malloc(strlen(home) + strlen(path));
        // Copy the home directory
        strcpy(newPath, home);
        // Copy the rest of the path
        strcat(newPath, path + 1);
        // Change the directory
        chdir(newPath);
        // Free the memory
        free(newPath);
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
            return; // Prevent the current directory from being added to the history
        }
    } else {

        if (::chdir(path) != 0)
        {
            std::cout << "Error: " << strerror(errno) << std::endl;
        }
        // Set the new path for history
    }
    // Add the new path to the history
    dirHistory.push_back(getcwd(NULL, 0));
}


// define action performed by detecting signal
void endSignalHandler(int signum) {
    int status;
    pid_t pid;

    if ((pid = waitpid( -1, &status, WNOHANG)) > 0) {
        printf("\nProcess %d exited\n", pid);

    }

}

std::string getPrefixes() {
    char *pwd = getcwd(NULL, 0);
    return "\033[1;32m" + std::string(pwd) + "\033[0m" + " " + "\033[1;31m" + std::to_string(getRuntime()) + "\033[0m" + " $ ";
}
int readCommand(char **com, char ***par)
{
    fprintf(stdout, "%s", getPrefixes().c_str());

    char *line = NULL;
    size_t len = 0;
    char *delim = " \n";
    // get line from stdin
    /* line = readline(getPrefixes().c_str()); */
    printf("Before getline");
    try{

    
    getline(&line, &len, stdin);
    printf("After getline");
    /* add_history(line); */
    char *tmp = strtok(line, delim);
printf("After strtok");
    int i = 0;

    // split line into parameters
    // allocate memory for parameters array
    *par = (char **)malloc(sizeof(char *) * 10);
    printf("After malloc");
    int background = 0;
    while (tmp != NULL)
    {

        // if last element is "&"
        if (strcmp(tmp, "&") == 0)
        {
            // set last element to NULL
            // set background flag
            (*par)[i] = NULL;
            background = 1;
            break;
        }
        else
        {

            (*par)[i] = tmp;
            i++;
            tmp = strtok(NULL, delim);
        }
    }
    printf("After while");
    *com = *par[0];
    printf("After com");
    return background;
	    }catch(...){
	printf("Error");
	return NULL;

    }
}

bool running = true;

void askForExit() {

    std::cout << "Are you sure you want to exit? (y/n)" << std::endl;
    char answer;
    std::cin >> answer;
    if (answer == 'y') {
        running = false;
    } else {
        return;
    }
};

int main(int argc, char *argv[])
{

    dirHistory.push_back(getcwd(NULL, 0));
    int childPid;
    char *command;
    char **parameters;


//Handle Sig for ctl c
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = sig_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);


    while (running)
    {
        /* std::cout << fs::current_path() << ">"; */
        int background = readCommand(&command, &parameters);
	printf("command end");
        std::cout << "Command: " << command << std::endl;
        // Command empty
        if (command == NULL)
        {
            continue;
        }
        // Check for own commands
        ownCommands ownCommand = getOwnCommand(command);
        switch (ownCommand)
        {
        case help:
        {   std::cout << "help" << std::endl;
            break;
        }
        case cd:
        {
            if (parameters[1] == NULL)
            {
                std::cout << "Error: No path specified" << std::endl;
                break;
            }
            changeDir(parameters[1]);
            break;
        }
        case EXIT: {
            askForExit();
            break;
        }
        default:
            if ((childPid = fork()) == -1)
            {
                std::cout << "ErrorFORK: " << strerror(errno) << std::endl;
                continue;
            }
            else if (childPid == 0)
            {
                // Child process
                if (execvp(command, parameters) == -1)
                {
                    std::cout << "ErrorEXECVP: " << strerror(errno) << std::endl;
                    break;
                }
            } else if(background) {
                char* tempCMD = command;
                std::cout << "Background process: " << childPid << " " << tempCMD << std::endl;
                signal(SIGCHLD, endSignalHandler);
            } else {
                // Parent process
                int status;
                if (waitpid(childPid, &status, 0) == -1)
                {
                    std::cout << "ErrorWAITPID: " << strerror(errno) << std::endl;
                    break;
                }
            }
        }
    }
}

