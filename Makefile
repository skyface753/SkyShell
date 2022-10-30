

build: 
	@echo "Building...";
	@g++ -std=c++17 -o skyshell main.cpp 
debug: 
	@echo "Debuging ...";
	@g++ -std=c++17 -g -o skyshell.debug main.cpp
	@lldb skyshell.debug 
