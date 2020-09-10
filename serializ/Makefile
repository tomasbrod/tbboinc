all: main run-main

run-main: main
	./main

main: main.cpp serialize.hpp Makefile
	g++ -g -O0 -std=c++03 -o $@ -Wall main.cpp -ltinyxml2
