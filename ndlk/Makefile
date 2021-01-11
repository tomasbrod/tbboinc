all: kanonb ortogonbw ortogonb dlkconv.exe ortowalk rules

ndlk: ndlk.cpp exact_cover_mt.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O0 -o $@ ndlk.cpp -lpthread

ortogonb: ortogonb.cpp exact_cover_mt.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ ortogonb.cpp -lpthread

isortogon: isortogon.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ isortogon.cpp

dlkconv.exe: dlkconv.cpp exact_cover_mt.cpp dlk_util.cpp nch10_glue.o Makefile
	g++ -std=c++11 -g -O3 -o $@ dlkconv.cpp

ortowalk: ortogowalk.cpp dlk_util.cpp exact_cover_mt.cpp kanonizer_v.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ ortogowalk.cpp -lpthread

kanonb2: kanonb2.cpp dlk_util.cpp exact_cover_mt.cpp kanonizer_b.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ kanonb2.cpp -lpthread

kanonb: kanonb.cpp dlk_util.cpp kanonizer_b.cpp kanonizer_v.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ kanonb.cpp -lpthread

izolist: izolist.cpp dlk_util.cpp kanonizer_b.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ izolist.cpp -lpthread

nch10_glue.o: nch10_glue.cpp Makefile dlk_util.cpp ../odlkcommon/namechdlk10.cpp
	g++ -c -std=c++11 -O3 -o $@ -I .. nch10_glue.cpp

rules: rules.cpp Makefile dlk_util.cpp kanonizer_v.cpp Makefile
	g++ -std=c++11 -O3 -g -o $@ rules.cpp

ortogonbw: ortogonbw.cpp exact_cover_mt.cpp dlk_util.cpp Makefile
	g++ -std=c++11 -g -O3 -o $@ ortogonbw.cpp -lpthread
