#include "SHELL.h"
#include <csignal>
#include <iostream>
#include "readline/readline.h"

SHELL::SHELL()
{
    // ctor
    this->startTime = std::chrono::system_clock::now();
    this->dirHistory.push_back(getcwd(NULL, 0));
}

int SHELL::getRuntime()
{
    std::chrono::time_point<std::chrono::system_clock> starTime = this->startTime;
    std::chrono::time_point<std::chrono::system_clock> endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = endTime - starTime;
    return elapsed_seconds.count();
}

SHELL::~SHELL()
{
    // dtor
    //  Print the runtime
    std::cout << "Runtime: " << this->getRuntime() << " seconds" << std::endl;
}

void SHELL::sig_handler(int s) {
    /* std::cin.ignore(); */
    /* ::exit(0); */
    // Flush
    /* std::cout << std::endl; */
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

void SHELL::changeDir(const char *path)
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
        if (this->dirHistory.size() > 1)
        {
            // Get the previous directory
            char *prevDir = this->dirHistory[this->dirHistory.size() - 2];
            // Change the directory
            ::chdir(prevDir);
            // Remove the previous directory from the history
            this->dirHistory.pop_back();
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
    this->dirHistory.push_back(getcwd(NULL, 0));
}

// define action performed by detecting signal
void SHELL::signalHandler(int signum) {
    printf("\nInterrupt signal (%d) received.\n", signum);

    ::exit(signum);
}

// define action performed by detecting signal
void SHELL::endSignalHandler(int signum) {
    int status;
    pid_t pid;

    if ((pid = waitpid( -1, &status, WNOHANG)) > 0) {
        printf("\nProcess %d exited\n", pid);

    }

}

int SHELL::readCommand(char **com, char ***par)
{
    char *line = NULL;
    size_t len = 0;
    char *delim = " \n";
    // get line from stdin
    line = readline(getPrefixes().c_str());
    // if line is empty, return 0
    if (line == NULL)
    {
        return 0;
    }

    /* getline(&line, &len, stdin); */
    add_history(line);
    char *tmp = strtok(line, delim);

    int i = 0;

    // split line into parameters
    // allocate memory for parameters array
    *par = (char **)malloc(sizeof(char *) * 100);
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

    *com = *par[0];
    return background;
}

std::string SHELL::getPrefixes() {

    fs::path currentPath = fs::current_path();
    // Last element of the path
    std::string lastElement = currentPath.filename().string();
    /* std::cout << "\033[1;31mbold red text\033[0m\n"; */
    /* return "\033[1;31mbold red text\033[0m\n"; */
    return "\033[1;32m" + currentPath.string() + "\033[0m" + " > ";
    /* std::cout << "\033[1;32m" << currentPath << "\033[0m" */
    /* 		  << " > " << lastElement << " > "; */
}

void SHELL::askForExit() {

    std::cout << "Are you sure you want to exit? (y/n)" << std::endl;
    char *answer = (char *)malloc(sizeof(char) * 100);
    std::cin >> answer;
    if (strcmp(answer, "y") == 0)
    {
        this->running = false;
        return;
    }
    else
    {
        return;
    }
};

void SHELL::run()
{
    // SigHandler Background
    std::signal(SIGINT, signalHandler);
    int childPid;
    char *command;
    char **parameters;


//Handle Sig for ctl c
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = sig_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);


    while (this->running)
    {
        /* std::cout << fs::current_path() << ">"; */
        int background = readCommand(&command, &parameters);
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
                /* std::cout << "RUN in foreground" << std::endl; */
                // Parent process
                int status;
                if (waitpid(childPid, &status, 0) == -1)
                {
                    std::cout << "ErrorWAITPID: " << strerror(errno) << std::endl;
                    break;
                }
                /* std::cout << "Child process exited with status: " << status << std::endl; */


            }
        }
        /* std::cin.seekg(0, std::ios::end); */
        /* { */
        /* 	if(background){ */

        /* 	} */
        /* 	// Parent process */
        /* 	if (background == 0) */
        /* 	{ */
        /* 		waitpid(childPid, NULL, 0); */
        /* 	} */
        /* } */
    }
}
