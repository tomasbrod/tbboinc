#include <stdio.h>
#include <string>
#include "parse2.hpp"
#include "parse4.hpp"
#include <fcgiapp.h>
#include "typese.hpp"
#include "kv.hpp"
#include "log.hpp"
#include <memory>
#include <thread>
#include "build/config-db.hpp"
#include "code.cpp"
using std::string;

void thrfunc(KVBackend* kv, unsigned ii)
{
	for(unsigned i=0; i< 1000; ++i ) {
		CStUnStream<4> key;
		key.wb4(ii+i);
		CStUnStream<4> val;
		val.w4(0x12345678);
		kv->Set(key,val);
	}
}

struct test1 {
	int& v1;
	int& v2;
};

int main(int argc, char** argv) {

	setlocale(LC_NUMERIC, "C");

	CLog::output = &std::cout;
	CLog::timestamps = 0;
	CLog log ( "main" );

	log("strtoul(7F,,16)=",strtoul("7F",NULL,17));
	string test = "zidan_";
	log(test.substr(6));
	int v1, v2;
	test1 test2 {v1,v2};

	unlink("try.db/data.mdb");

	t_config_database cfg = {
		.name = "trydb",
		.type = t_config_database::v_kyotoh,
		.path = "try.kdb",
		.mmap= 1,
		.sync= 2,
		.block= 0,
		.memtable= 0,
		.sstable= 0,
		.cache= 0,
		.bloom= 0,
		.checksum= -1,
		.compress= -1,
		.small= 0,
		.linear= 0,
		.kyotofbp= 0,
		.buckets= 0,
		.defrag= 0,
	};

	CStUnStream<64> bin;
	for(size_t i=0; i<32; ++i) bin.w1(255-i);
	string res58 = BSNCode::base58enc(bin,44);
	log("58",res58);
	
	log("Opening ",cfg.path);
	std::unique_ptr<KVBackend> kv = OpenKV(cfg);
	kv->Commit();
	log("begin");
	/*
	std::thread th1 ( thrfunc, kv.get(), 0 );
	std::thread th2 ( thrfunc, kv.get(), (1<<24) );
	th1.join();
	th2.join();
	*/
	thrfunc(kv.get(), (2<<24));
	/*log("aborting");
	abort();*/
	log("commiting");
	kv->Commit();
	log("done");

	return 0;
}

