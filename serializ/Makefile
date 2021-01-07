all: main run-main

run-main: main
	./main

main: sertest.cpp Makefile
	g++ -g -O0 -std=c++03 -o $@ -Wall -I ../boinc/lib -I ../boinc sertest.cpp ../boinc/lib/libboinc.a
