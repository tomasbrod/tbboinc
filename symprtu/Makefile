all: sievetest spt generate1 asim1 stuff fuh sample.dat

scpn1: scpn1.cpp
	g++ -g -o $@ -O1 scpn1.cpp -lprimesieve

sievetest: sievetest.cpp
	g++ -g -o $@ -O1 -I primesieve/include sievetest.cpp primesieve/libprimesieve.so

spt: spt.cpp wio.cpp Makefile
	g++ -o $@ -static -O3 -I ../boinc/api -I ../boinc/lib -I ../boinc -I primesieve/include spt.cpp primesieve/libprimesieve.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc.a -lpthread

stuff: stuff.cpp wio.cpp
	g++ -g -o $@ -O0 -I primesieve/include -I ../boinc/api -I ../boinc/lib -I ../boinc stuff.cpp primesieve/libprimesieve.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc.a -lpthread

generate1: generate1.cpp ../bocom/Stream.cpp ../bocom/Wiodb.cpp wio.cpp Makefile
	g++ -o $@ -g -O0 -I .. -I .. -I ../boinc/api -I ../boinc/lib -I ../boinc/tools -I ../boinc/sched -I ../boinc/db -I /usr/include/mysql -I ../boinc generate1.cpp ../boinc/sched/libsched.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc.a ../boinc/lib/libboinc_crypt.a -lmysqlclient -lpthread -lssl -lcrypto

asim1: asim.cpp ../bocom/Stream.cpp ../bocom/Wiodb.cpp wio.cpp Makefile
	g++ -o $@ -g -O1 -I primesieve/include -I .. -I .. -I ../boinc/api -I ../boinc/lib -I ../boinc/tools -I ../boinc/sched -I ../boinc/db -I /usr/include/mysql -I ../boinc asim.cpp ../boinc/sched/libsched.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc.a ../boinc/lib/libboinc_crypt.a  primesieve/libprimesieve.a -lmysqlclient -lpthread -lssl -lcrypto

asimO: asim_old.cpp Stream.cpp wio.cpp Makefile
	g++ -o $@ -g -O1 -I .. -I .. -I ../boinc/api -I ../boinc/lib -I ../boinc/tools -I ../boinc/sched -I ../boinc/db -I /usr/include/mysql -I ../boinc asim_old.cpp ../boinc/sched/libsched.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc.a ../boinc/lib/libboinc_crypt.a -lmysqlclient -lpthread -lssl -lcrypto

sample.dat: stuff
	./stuff -s

output.dat: sample.dat spt
	cp sample.dat input.dat
	./spt

fuh: file_upload_handler.cpp Makefile
	g++ -o $@ -g -O0 -I .. -I .. -I ../boinc/api -I ../boinc/lib -I ../boinc/tools -I ../boinc/sched -I ../boinc/db -I /usr/include/mysql -I ../boinc -D_USING_FCGI_ file_upload_handler.cpp ../boinc/sched/fcgi_file_upload_handler-sched_config.o ../boinc/sched/libsched.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc_fcgi.a ../boinc/lib/libboinc_crypt.a -lmysqlclient -lpthread -lssl -lcrypto -lfcgi
