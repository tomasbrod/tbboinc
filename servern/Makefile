all: servern

CXX = g++ -std=c++11 -Wformat -Wno-format-truncation

servern: build/servern.o build/kv.o build/Stream.o build/parse.o build/group.o build/COutput.o build/tag.o build/code.o build/handle.o build/md5.o
	$(CXX) -o $@ -g -O0 $^ -lkyotocabinet -llmdb -lsodium -lleveldb

dbtry: build/dbtry.o build/kv.o build/Stream.o
	$(CXX) -o $@ -g -O0 $^ -llmdb -lkyotocabinet -lleveldb -lpthread -lrt

show_keys: build/show_keys.o build/boinclib.o build/Stream.o build/parse.o
	$(CXX) -o $@ -g -O0 $^ -lsodium -lssl -lcrypto

build/boinclib.o: boinclib.cpp Makefile
	$(CXX) -c -o $@ -g -O0 -I boinclib $<

build/%.o: %.cpp Makefile
	$(CXX) -c -o $@ -g -O0 $<

build/md5.o: ../boinc/lib/md5.cpp
	$(CXX) -c -o $@ $<

build/%.hpp build/%.cpp: %.defs defs.php Makefile
	php defs.php $<

fuh: file_upload_handler.cpp Makefile
	$(CXX) -o $@ -g -O0 -I .. -I .. -I ../boinc/api -I ../boinc/lib -I ../boinc/tools -I ../boinc/sched -I ../boinc/db -I /usr/include/mysql -I ../boinc -D_USING_FCGI_ file_upload_handler.cpp ../boinc/sched/fcgi_file_upload_handler-sched_config.o ../boinc/sched/libsched.a ../boinc/api/libboinc_api.a ../boinc/lib/libboinc_fcgi.a ../boinc/lib/libboinc_crypt.a -lmysqlclient -lpthread -lssl -lcrypto -lfcgi


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
build/kv.o: kv.cpp kv.hpp typese.hpp log.hpp build/config.hpp Stream.hpp 
build/parse.o: parse.cpp parse.hpp Stream.hpp
build/servern.o: servern.cpp parse.hpp Stream.hpp typese.hpp log.hpp kv.hpp \
 build/config.hpp build/config.cpp build/request.hpp \
 build/request.cpp COutput.hpp tag.hpp group.hpp dump.cpp
build/Stream.o: Stream.cpp Stream.hpp log.hpp
build/COutput.o: COutput.hpp COutput.cpp typese.hpp Stream.hpp log.hpp kv.hpp group.hpp build/config.hpp build/config.cpp
build/group.o: group.cpp group.hpp kv.hpp typese.hpp log.hpp Stream.hpp build/config.hpp tag.hpp
build/tag.o: tag.hpp Stream.hpp
build/dbtry.o: tag.hpp Stream.hpp typese.hpp log.hpp kv.hpp \
 build/config.hpp tag.hpp code.cpp
build/handle.o: handle.cpp
.PHONY: handle.cpp
