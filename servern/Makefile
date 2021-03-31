all: servern

servern: build/servern.o build/boinclib.o build/kv.o build/Stream.o build/parse2.o
	g++ -o $@ -g -O0 $^ -lkyotocabinet -llmdb -lsodium -lssl -lcrypto

all: build/defs.hpp

build/boinclib.o: boinclib.cpp Makefile
	g++ -c -o $@ -g -O0 -I boinclib $<

build/%.o: %.cpp Makefile
	g++ -c -o $@ -g -O0 $<

build/defs.hpp build/defs.cpp build/cofig.hpp: defs.txt defs.php config.defs Makefile
	php defs.php defs.txt
	php defs.php config.defs

fuh: file_upload_handler.cpp Makefile
	g++ -o $@ -g -O0 -I .. -I .. -I ../boinc/api -I ../boinc/lib -I ../boinc/tools -I ../boinc/sched -I ../boinc/db -I /usr/include/mysql -I ../boinc -D_USING_FCGI_ file_upload_handler.cpp ../boinc/sched/fcgi_file_upload_handler-sched_config.o ../boinc/sched/libsched.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc_fcgi.a ../boinc/lib/libboinc_crypt.a -lmysqlclient -lpthread -lssl -lcrypto -lfcgi


# todo:
# bring in basic fcgi server from boinc_new

# note about fcgi - 

# -MF boinclib.d -MM
build/boinclib.o: boinclib.cpp boinclib/miofile.cpp boinclib/config.h \
 boinclib/version.h boinclib/error_numbers.h boinclib/str_replace.h \
 boinclib/miofile.h boinclib/mfile.h boinclib/mfile.cpp \
 boinclib/filesys.h boinclib/str_util.cpp boinclib/common_defs.h \
 boinclib/parse.h boinclib/str_util.h boinclib/filesys.cpp \
 boinclib/util.h boinclib/util.cpp boinclib/base64.h boinclib/parse.cpp
build/kv.o: kv.cpp Stream.hpp kv.hpp
build/servern.o: servern.cpp boinclib/miofile.h boinclib/mfile.h \
 parse2.hpp Stream.hpp build/cofig.hpp build/defs.hpp parse2.hpp
build/Stream.o: Stream.cpp Stream.hpp
