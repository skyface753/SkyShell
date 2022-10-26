#ifndef SHELL_H
#define SHELL_H
#include <iostream>
#include <string>
#include <chrono>
#include <filesystem>
namespace fs = std::filesystem;
#include <unistd.h>
#include <stdlib.h> 
#include<readline/history.h>
#include <signal.h>

class SHELL

{
public:
    SHELL();
    ~SHELL();
    void run();
   int getRuntime();
void exit();
private:
bool running = true;
static void signalHandler(int signum);
static void endSignalHandler(int signum);
void changeDir(const char *path);
int readCommand(char **command, char ***par);
// Starttime
	   std::chrono::time_point<std::chrono::system_clock> startTime;
};

#endif // SHELL_H

