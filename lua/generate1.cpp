/* Generator */
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <bitset>
#include <ctime>
#include <unistd.h>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>


#include "boinc_api.h"

using std::vector;
using std::cerr;
using std::endl;

#include "config.h"
#include "backend_lib.h"
#include "error_numbers.h"
#include "sched_config.h"
#include "sched_util.h"
#include "validate_util.h"
#include "credit.h"
#include "md5_file.h"

struct EApp : std::runtime_error { using std::runtime_error::runtime_error; };
struct EBoincApi : std::exception {
	int retval;
	const char* msg;
	std::string msg2;
	EBoincApi(int _retval, const char* _msg)
		:retval(_retval), msg(_msg)
	{
		std::stringstream ss;
		ss<<"boinc_api: "<<msg<<": "<<boincerror(retval);
		msg2=ss.str();
	}
	const char * what () const noexcept {return msg2.c_str();}
};
struct EDatabase	: std::runtime_error { using runtime_error::runtime_error; };
struct EInvalid	: std::runtime_error { using runtime_error::runtime_error; };
static int retval;

#include "../bocom/Stream.cpp"
#include "../bocom/Wiodb.cpp"

DB_APP lua_app;


void initz() {
	int retval = config.parse_file();
	if (retval) {
			log_messages.printf(MSG_CRITICAL,
					"Can't parse config.xml: %s\n", boincerror(retval)
			);
			exit(1);
	}

	retval = boinc_db.open(
			config.db_name, config.db_host, config.db_user, config.db_passwd
	);
	if (retval) {
			log_messages.printf(MSG_CRITICAL,
					"boinc_db.open failed: %s\n", boincerror(retval)
			);
			exit(1);
	}
	if (lua_app.lookup("where name='lua8'")) {
		std::cerr<<"can't find app lua8\n";
		exit(4);
	}
}

void copy_file_if_not_exists(const char* src, const std::string& dest)
{
	int out = open(dest.c_str(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IRGRP | S_IROTH);
	if(out<0 && errno!=EEXIST)
		throw EDatabase("open(outfile) failed");
	if(out<0 && errno==EEXIST)
		return;
	int inp = open(src, O_RDONLY);
	if(inp<0)
		throw EDatabase("open(inpfile) failed");
	char buf[4096];
	size_t rl, wl;
	do {
		rl= read(inp, buf, sizeof buf);
		if(!rl) break;
		if(rl<0) throw EDatabase("read(inpfile) failed");
		wl = write(out, buf, rl);
		if(wl!=rl) throw EDatabase("write(outfile) failed");
	} while(rl==sizeof buf);
	close(out);
	close(inp);
}

struct file_desc {
	const char* name;
	const char* open;
	bool copy;
};


void build_xml_doc(DB_WORKUNIT &wu, const std::string input_name, const struct file_desc const_files[])
{
	std::stringstream xml;
	std::vector<std::string> hashes;
	int retval;
	for(unsigned i=0; const_files[i].name || const_files[i].open; ++i) {
		char in_md5[256];
		double nbytes = 0;
		if(const_files[i].name) {
			retval = md5_file(const_files[i].name, in_md5, nbytes,0);
			copy_file_if_not_exists(const_files[i].name, std::string(config.download_dir)+"/"+std::string(in_md5));
			hashes.push_back(in_md5);
			xml<<"<file_info>\n<name>"<<in_md5
			<<"</name>\n<url>https://boinc.tbrada.eu/download/"<<in_md5;
		} else {
			std::string fn = (std::string(config.download_dir)+"/"+input_name);
			retval = md5_file(fn.c_str(), in_md5, nbytes,0);
			hashes.push_back(input_name);
			xml<<"<file_info>\n<name>"<<input_name
			<<"</name>\n<url>https://boinc.tbrada.eu/download/"<<in_md5;
		}
		if(retval) throw EDatabase("md5_file failed");
		xml
		<<"</url>\n<md5_cksum>"<<in_md5<<"</md5_cksum>\n<nbytes>"<<nbytes
		<<"</nbytes>\n</file_info>\n";
		if(!const_files[i].name) break;
	}
	xml<<"<workunit>\n";
	for(unsigned i=0; i<hashes.size(); ++i) {
		xml<<"<file_ref>\n<file_name>"<<hashes[i]<<"</file_name>\n<open_name>";
		if(const_files[i].open) {
			xml<<const_files[i].open;
		} else {
			xml<<const_files[i].name;
		}
		xml<<"</open_name>\n";
		if(const_files[i].copy) {
			xml<<"<copy_file/>\n";
		}
		xml<<"</file_ref>\n";
	}
	xml<<"</workunit>";
	size_t len = xml.str().size();
	if(len>sizeof wu.xml_doc)
		throw EDatabase("xml_doc too big");
	memcpy(wu.xml_doc, xml.str().c_str(), len);
}

void submit_wu_in(std::ifstream& todof, unsigned& cntr)
{
	std::stringstream wuname;
	DB_WORKUNIT wu; wu.clear();


		wu.appid = lua_app.id;
		wu.rsc_memory_bound = 399e6;
		wu.rsc_disk_bound = 1e8; //todo 100m
		wu.delay_bound = 1 * 24 * 3600;
		wu.priority = 25;
		wu.batch= 74;
		wu.target_nresults= wu.min_quorum = 2;
		wu.max_error_results= wu.max_total_results= 32;
		wu.max_success_results= 8;

	wuname<<"lua8_"<<wu.batch<<"_"<<cntr;
	std::cout<<" WU "<<wuname.str()<<endl;

	std::stringstream fninp;
	fninp<<config.download_dir<<"/"<<wuname.str()<<".in";
	std::ofstream infileh(fninp.str());
	unsigned lines = 0;
	const unsigned max_lines = 80;
	while(todof && lines<max_lines) {
		std::string line;
		getline(todof, line);
		if(line.empty()) break;
		infileh<<line<<endl;
		lines++;
	}
	if(!lines) return;
	infileh.close();

		//14e12 is one hour on mangan-pc
		wu.rsc_fpops_est = 16e12 / 60.0 * lines;
		wu.rsc_fpops_bound = wu.rsc_fpops_est * 24;

	strcpy(wu.name, wuname.str().c_str());

	const struct file_desc const_files[]= {
		"library.lua",0,0,
		"try.lua","driver.lua",0,
		"kanon_app.cpp",0,1,
		"dlk_util.cpp",0,1,
		"kanonizer_b.cpp",0,1,
		0,"input.txt",1
		};
	build_xml_doc(wu, wuname.str()+".in", const_files);

	wu.transitioner_flags = 2; //?
	retval= create_work4(wu,"templates/lua8_simple_out",config);
	if(retval) throw EDatabase("create_work4 failed");
}

int main(int argc, char** argv) {

	//node: min app version is set in app table
	initz();
	if(boinc_db.start_transaction())
		exit(4);

	unsigned count = 0;
	std::ifstream todof ( "kan_todo.txt" );
	while(todof) {
		count++;
		submit_wu_in(todof, count);
	}
	cerr<<"Count: "<<count<<endl;

	if(boinc_db.commit_transaction()) {
		cerr<<"failed to commit transaction"<<endl;
		exit(1);
	}
	
	boinc_db.close();
	return 0;
}

