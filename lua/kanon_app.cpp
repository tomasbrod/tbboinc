#include <cstddef>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <list>
#include <error.h>
#include <thread>
#include <mutex>
using std::string;
using std::endl;

#include "boinc/boinc_api.h"
#include "dlk_util.cpp"
#include "kanonizer_b.cpp"

Kanonizer kanonizer;

std::string Work(const std::string& enc)
{
	Square sq;
	sq.Decode(enc);
	if(!sq.width()) throw std::runtime_error("Zero-width square");
	Square min = kanonizer.Kanon(sq);
	return min.Encode();
}

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

int main(int argc, char* argv[])
{
	
	int retval = boinc_init();
	if(retval) 
			throw EBoincApi(retval, "boinc_init");
	std::string input_fn, output_fn;
	retval= boinc_resolve_filename_s("input.txt",input_fn);
	if(retval) throw EBoincApi(retval,"boinc_resolve_filename_s");
	retval= boinc_resolve_filename_s("output.txt",output_fn);
	if(retval) throw EBoincApi(retval,"boinc_resolve_filename_s");

	try {
		std::ifstream input (input_fn);
		std::ofstream output (output_fn);
		while(input) {
			std::string line;
			std::getline(input,line);
			if( line!="" && line[0]!=' ' && line[0]!='#' ) {
				output<<Work(line)<<endl;
			}
		}
	}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 1;
	}
	boinc_finish(0);
	return 0;
}
