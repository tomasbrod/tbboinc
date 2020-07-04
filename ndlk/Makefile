all: ndlk dlkconv.exe ortogonb isortogon

ndlk: ndlk.cpp exact_cover_mt.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O0 -o $@ ndlk.cpp -lpthread

ortogonb: ortogonb.cpp exact_cover_mt.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ ortogonb.cpp -lpthread

isortogon: isortogon.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ isortogon.cpp

dlkconv.exe: dlkconv.cpp exact_cover_mt.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ dlkconv.cpp
