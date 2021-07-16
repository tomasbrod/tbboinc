#include <stdio.h>
#include <string>
#include <algorithm>
#include <sodium/crypto_auth.h>
#include "parse.hpp"
#include "tag.hpp"
#include <fcgiapp.h>
#include "typese.hpp"
#include "kv.hpp"
#include "build/request.hpp"
#include "log.hpp"
#include <memory>
#include "group.hpp"
#include "COutput.hpp"
#include "build/config.hpp"
#include "build/config.cpp"
#include "../boinc/lib/md5.h"
using std::string;

void parse_test(FILE* f);
void parse_test_3(FILE* f);
void kv_test();

struct CConfig : t_config
{
} config;

#include "dump.cpp"

class CKeys
{
	// server_keys, codesign, oldname, group
	std::vector<std::array<byte,32>> master_keys;
	// hex key to bin
	//the upload keys will be pre-derived
	//peer and plugin keys will be on-demand
	std::vector<uint64_t> sig_ids;
	public:
	static const immstring<9> prefix;
	void derive(std::array<byte,32>& dest, const CBuffer info);
	std::vector<std::array<byte,32>> upload_keys;
	std::vector<Dings> expire;
	//returns -1 on current, -2 on error, or other if signature exists
	long findSignature( const string& icodesig );
	std::vector<std::string> signatures;
	std::string codesign;
	public:
	void init(CLog& log, CConfig& config);
};

const immstring<9> CKeys::prefix = "OINCSrvN"; // BONCI?

static string tohex(const std::array<byte,32>& key)
{
	CStUnStream<64> buf;
	buf.writehex(key.data(),32);
	return string((const char*)buf.base,buf.pos());
}

long CKeys::findSignature( const string& ikey )
{
	if ( ikey == codesign )
		return -1;
	else {
		md5_state_t md5s;
		md5_byte_t digest[16];
		const char* tail = "\n";
		md5_init(&md5s);
		md5_append(&md5s, (const byte*)ikey.data(), ikey.size());
		//md5_append(&md5s, (const byte*)tail, 1);
		/* since the certificate is made from the stripped version,
		 * md5sum of the stripped version also makes sense
		*/
		md5_finish(&md5s, digest);
		const auto pos = std::find(sig_ids.begin(),sig_ids.end(),*((uint64_t*)digest));
		if(pos==sig_ids.end())
			return -2;
		return (pos-sig_ids.begin());
	}
}

static const string codesign_test =
"1024\n"
"ddc619f8e6c71dcecc4c017cea624f34d4ab166a92505aa399935b1d002d8333\n"
"f66681eaf3a20715745bf353f2e2bcf6f5acef0bd58aa48c90c4b08ec43aad05\n"
"af022c883486bcad97aba77db301bc4952d676e0b777016b6e20b98cb65d29e4\n"
"064f941097d66d1660eaa6d83ba00428368b7094f1a433eaa828e0506ed34c33\n"
"0000000000000000000000000000000000000000000000000000000000000000\n"
"0000000000000000000000000000000000000000000000000000000000000000\n"
"0000000000000000000000000000000000000000000000000000000000000000\n"
"0000000000000000000000000000000000000000000000000000000000010001\n"
".";

void CKeys::init(CLog& log, CConfig& config)
{
	CStUnStream<10> info;
	info.wstrf(prefix);
	info.w2(COutput::type_id);
	assert((crypto_auth_hmacsha512256_BYTES==crypto_auth_hmacsha512256_KEYBYTES)&&(crypto_auth_hmacsha512256_BYTES==32));
	master_keys.clear();
	expire.clear();
	assert(config.server_keys.master.size()==64);
	expire.push_back(Dings::max());
	master_keys.emplace_back();
	assert(CBuffer::hex2bin(master_keys.back().data(), config.server_keys.master.data(), 64));
	for(const t_config_server_keys_old& ok : config.server_keys.old) {
		assert(ok.content.size()==64);
		expire.emplace_back( ok.expire/64 );
		master_keys.emplace_back();
		assert(CBuffer::hex2bin(master_keys.back().data(), ok.content.data(), 64));
	}
	upload_keys.resize(master_keys.size());
	for(unsigned ix=0; ix<master_keys.size(); ++ix) {
		crypto_auth_hmacsha512256(
			upload_keys.at(ix).data(),
			info.base,info.pos(),
			master_keys.at(ix).data()
		);
	}
	config.server_keys.master.clear();
	config.server_keys.old.clear();
	sig_ids.resize(config.signature.size());
	signatures.resize(config.signature.size());
	//config.signature.id is truncated md5 of the public key
	for(unsigned i=0; i < config.signature.size(); ++i)
	{
		uint64_t& id = sig_ids[i];
		assert(CBuffer::hex2bin((byte*)&id,config.signature[i].id,16));
		signatures[i] = std::move(config.signature[i].content);
	}
	codesign = std::move(config.codesign);
	//log("codesign_test",findSignature(codesign_test));
	//log("codesign_test",findSignature(std::string(codesign)));
	//log("codesign_test",findSignature(std::string("Blah")));
	// signatures is stored without the final "\n.\n"
	// codesign   is stored without the final "\n.\n"
}

void CKeys::derive(std::array<byte,32>& dest, const CBuffer info)
{
	assert(info.pos()>2);
	assert(0==strncmp((char*)info.base,prefix,2));
	crypto_auth_hmacsha512256(dest.data(), info.base,info.pos(), master_keys.at(0).data());
}

std::chrono::steady_clock::time_point tick_epoch;

int main(int argc, char** argv) {

	setlocale(LC_NUMERIC, "C");

	CLog::output = &std::cout;
	CLog::timestamps = 0;
	CLog log ( "main" );
	

	tick_epoch = std::chrono::steady_clock::now() - std::chrono::minutes(1);

	const char* reload_path;
	bool reload_xml;
	const char* dump_path = "dump.hex";
	{
		std::string config_path = "config.xml";
		// parse command line
		for(unsigned ai=1; ai<argc; ++ai)
		{
			if(!strncmp(argv[ai],"c=",2)) {
				config_path = argv[ai]+2;
			}
			else log.warn("Unknown option",argv[ai]);
			/*
			if(!strncmp(argv[ai],"load=",5)) {
				reload_path = argv[ai]+2;
				reload_xml = 1;
			}
			if(!strncmp(argv[ai],"lhex=",5)) {
				reload_path = argv[ai]+2;
				reload_xml = 0;
			}
			*/
		}

		// read config
		try {
			log("Reading",config_path);
			CFileStream fs;
			fs.openRead(config_path.c_str());
			XmlParser xp (&fs);
			xp.get_tag();
			config.parse(xp);
			if(!config.has_keys) {
				log("Reading","keys.xml");
				fs.openRead("keys.xml");
				XmlParser xp2 (&fs);
				xp2.get_tag();
				config.server_keys.parse(xp2);
			}
		} catch(std::exception& e) {
			log.error(e, "while reading config file");
			return 1;
		}
		#if 0
		log("XML_TAG_test()");
		CLog::output->flush();
		XML_TAG_test();
		#endif
	}
	//Note: the config defines a dataoobject, while we extend it with runtime values
	// this runtime object is referenced from t_config.


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
	CKeys keys;
	keys.init(log, config);

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

	{
		log("Dumping");
		CLog::output->flush();
		CHandleStream hs (STDOUT_FILENO, true);
		dumpxml(hs);
	}
	if(dump_path) {
		log("Dumping hex",dump_path);
		CFileStream hs;
		hs.openCreate(dump_path);
		dumphex(hs);
	}

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
