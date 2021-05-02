#include <stdio.h>
#include <string>
#include "parse2.hpp"
#include <fcgiapp.h>
#include "build/config.hpp"
#include "build/config.cpp"
#include "build/request.hpp"
#include "build/request.cpp"
#include "kv.hpp"
#include "log.hpp"
#include <memory>
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
		log("Reading","config.xml");
		CFileStream fs;
		//todo: allow cmdline override or standard path
		fs.openRead("config.xml");
		XML_PARSER2 xp (&fs);
		xp.get_tag();
		config.parse(xp);
		if(!config.has_keys) {
			log("Reading","keys.xml");
			fs.openRead("keys.xml");
			XML_PARSER2 xp2 (&fs);
			xp2.get_tag();
			config.server_keys.parse(xp2);
		}
	} catch(std::exception& e) {
		log.error(e, "while reading config file");
		return 1;
	}


	//Note: the config defines a dataoobject, while we extend it with runtime values
	// this runtime object is referenced from t_config.

	// readonly FileStream done. We need:
	log("Hello", 69, 420, true, "world");
	EStreamOutOfBounds test;
	log.error(test, "while testing");

	// way to share global objects
	// all runtime objects should live under Config.

	// initialize database here
	// a function that based on db.type instantiates and opens the database class
	// or throws if unknown
	// we should also create the dao from the database

	//LogKV = CLog("db");

	//KVBackend kv;
	//kv.Open(config.database.path.c_str());
	std::vector<std::unique_ptr<KVBackend>> databases;
	try {
		for(const t_config_database& cfg : config.database) {
			log("Opening database",cfg.enum_table[cfg.type],cfg.path);
			databases.emplace_back(OpenKV(cfg));
			//databases.back()->Close();
		}
	} catch(std::exception& e) {
		log.error(e, "while opening databases");
		return 1;
	}

	databases.clear();

	// create plugin objects

	// authentiacte the request

	// stuff results in db


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
