

build: 
	@echo "Building...";
	@g++ -std=c++17 -o skyshell.out main.cpp shell.cpp -lreadline
