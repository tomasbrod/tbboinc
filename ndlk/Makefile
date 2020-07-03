all: ndlk dlkconv.exe

ndlk: ndlk.cpp exact_cover_mt.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O0 -o $@ ndlk.cpp -lpthread

dlkconv.exe: dlkconv.cpp exact_cover_mt.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ dlkconv.cpp
