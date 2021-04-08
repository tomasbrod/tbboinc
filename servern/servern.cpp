#include <stdio.h>
#include <string>
#include "parse2.hpp"
#include <fcgiapp.h>
#include "build/cofig.hpp"
#include "build/cofig.cpp"
#include "build/request.hpp"
#include "build/request.cpp"
#include "kv.hpp"
#include "log.hpp"
using std::string;

void parse_test(FILE* f);
void parse_test_3(FILE* f);
void kv_test();

t_config config;

int main(void) {

	CLog::output = &std::cout;
	CLog::timestamps = 0;
	CLog log ( "test" );
	CLog log2;
	log2.init ( log, "blah");
	log2("log2 hello");
	CLog log3;
	log3.init ( "%s.hlab", log.ident_cstr());
	log3("log3 hello");

	// read config
	try {
		CFileStream fs;
		//todo: allow cmdline override or standard path
		fs.openRead("config.xml");
		XML_PARSER2 xp (&fs);
		xp.get_tag();
		config.parse(xp);
	} catch(std::exception& e) {
		log.error(e, "while reading config file");
		return 1;
	}

	// readonly FileStream done. We need:
	// log support
	// log object that contains prepared prefix
	// datetime goes before the prefix if enabled globally
	// look at gridcoin log
	log("Hello", 69, 420, true, "world");
	EStreamOutOfBounds test;
	log.error(test, "while testing");

	// way to share global objects
	// just make them global, yolo

	// initialize database here
	// a function that based on db.type instantiates and opens the database class
	// or throws if unknown
	// we should also create the dao from the database

	//KVBackend kv;
	//kv.Open(config.database.path.c_str());


	// if stdio mode
	// open streams and call request processing
	// else abort

	//kv_test();

	// run scheduler in stdio mode
	CHandleStream clientin(STDIN_FILENO);
	CHandleStream clientout(STDOUT_FILENO);
	printf(".\n");

	return 1;
}
