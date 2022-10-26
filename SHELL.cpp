#include "SHELL.h"
#include <csignal>
#include <iostream>
#include "readline/readline.h"

SHELL::SHELL()
{
	// ctor
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

void SHELL::changeDir(const char *path)
{
	if (::chdir(path) != 0)
	{
		std::cout << "Error: " << strerror(errno) << std::endl;
	}
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
    
    if ((pid = waitpid( -1 , &status, WNOHANG)) > 0) {
        printf("\nProcess %d exited\n", pid);

    }

}

int SHELL::readCommand(char **com, char ***par)
{
	char *line = NULL;
	size_t len = 0;
	char *delim = " \n";
	// get line from stdin
	line = readline("shell> ");
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

void SHELL::run()
{
	std::signal(SIGINT, signalHandler);
	int childPid;
	char *command;
	char **parameters;
       bool running = true;
	while (running)
	{
		fs::path currentPath = fs::current_path();
		// Last element of the path
		std::string lastElement = currentPath.filename().string();
		std::cout << "\033[1;32m" << currentPath << "\033[0m"
				  << " > " << lastElement << " > ";
		/* std::cout << fs::current_path() << ">"; */
		int background = readCommand(&command, &parameters);
		// Command empty 
		if (command == NULL)
		{
			continue;
		}
		// Command exit
		if (strcmp(command, "exit") == 0)
		{
			std::cout << "Are you sure you want to exit? (y/n)" << std::endl;
			char *answer;
			std::cin >> answer;
			if (strcmp(answer, "y") == 0)
			{
				running = false;
				continue;
			}
			else
			{
				continue;
			}
		}
		// Change Dir command
		if (strcmp(command, "cd") == 0)
		{
			if (parameters[1] == NULL)
			{
				std::cout << "Error: No path specified" << std::endl;
				continue;
			}
			changeDir(parameters[1]);
			continue;
		}
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
		}else if(background){
			char* tempCMD = command;
			std::cout << "Background process: " << childPid << " " << tempCMD << std::endl;
			signal(SIGCHLD, endSignalHandler);
		}else{
			std::cout << "RUN in foreground" << std::endl;
                        			// Parent process
			int status;
			if (waitpid(childPid, &status, 0) == -1)
			{
				std::cout << "ErrorWAITPID: " << strerror(errno) << std::endl;
				break;
			}
			std::cout << "Child process exited with status: " << status << std::endl;


		}

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
