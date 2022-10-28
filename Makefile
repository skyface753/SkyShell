

build: 
	@echo "Building...";
	@g++ -std=c++17 -o skyshell.out mainNew.cpp 
	# @g++ -std=c++17 -o skyshell.out main.cpp -lreadline
debug: 
	@echo "Debuging ...";
	@g++ -std=c++17 -g -o skyshell.debug main.cpp -lreadline
	@lldb skyshell.debug 
