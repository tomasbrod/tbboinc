#include <stdio.h>
#include <string>
#include "parse2.hpp"
#include <fcgiapp.h>
#include "typese.hpp"
#include "kv.hpp"
#include "build/request.hpp"
#include "build/request.cpp"
#include "log.hpp"
#include <memory>
#include "kv-grp.cpp"
#include "COutput.cpp"
#include "build/config.hpp"
#include "build/config.cpp"
using std::string;

void parse_test(FILE* f);
void parse_test_3(FILE* f);
void kv_test();

struct CConfig : t_config
{
	GroupCtl group;
} config;

std::chrono::steady_clock::time_point tick_epoch;

int main(void) {

	CLog::output = &std::cout;
	CLog::timestamps = 0;
	CLog log ( "test" );
	CLog log2 ( log, "blah");
	log2("log2 hello");
	CLog log3 ( "%s.hlab", log.ident_cstr());
	log3("log3 hello");

	tick_epoch = std::chrono::steady_clock::now() - std::chrono::minutes(1);

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
	try {
		config.group.init(LogKV, config.database);
	} catch(std::exception& e) {
		log.error(e, "while opening databases");
		return 1;
	}

	config.group.Open();

	for( auto& plugin : config.plugin ) {
		plugin.init(config.group);
		for( auto& output : plugin.output ) {
			output.init(&plugin, config.group);
		}
	}
	config.group.open_since=Ticks::zero();
	config.group.Close();

	// create plugin objects

	// authentiacte the request

	// stuff results in db


	// if stdio mode
	// open streams and call request processing
	// else abort

	//kv_test();

	config.group.dbs.clear();

	// run scheduler in stdio mode
	CHandleStream clientin(STDIN_FILENO);
	CHandleStream clientout(STDOUT_FILENO);
	printf(".\n");

	return 1;
}

Ticks now()
{
	Ticks ticks = std::chrono::duration_cast<Ticks> ( std::chrono::steady_clock::now() - tick_epoch );
	//LogKV("now",ticks.count());
	return std::chrono::duration_cast<Ticks> ( std::chrono::steady_clock::now() - tick_epoch );
}
