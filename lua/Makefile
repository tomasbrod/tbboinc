all: driver driver_boinc.exe generate1

driver: driver.cpp Makefile
	g++ -o $@ -O0 -g driver.cpp -llua -lpthread

driver_boinc.exe: driver.cpp Makefile
	g++ -o $@ -O3 -g -DWITH_BOINC -static -I ../boinc/api -I ../boinc/lib -I ../boinc -I lua-5.3.5/src driver.cpp ../boinc/api/libboinc_api.a ../boinc/lib/libboinc.a lua-5.3.5/src/liblua.a -lpthread

generate1: generate1.cpp Makefile
	g++ -o $@ -g -O0 -I .. -I .. -I ../boinc/api -I ../boinc/lib -I ../boinc/tools -I ../boinc/sched -I ../boinc/db -I /usr/include/mysql -I ../boinc generate1.cpp ../boinc/sched/libsched.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc.a ../boinc/lib/libboinc_crypt.a -lmysqlclient -lpthread -lssl -lcrypto
