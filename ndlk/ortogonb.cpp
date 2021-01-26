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
#include <atomic>
using std::string;
using std::endl;

#include "dlk_util.cpp"
#include "exact_cover_mt.cpp"

int main(int argc, char* argv[])
{
	if(argc<=1) {
		std::cerr<<
			"ortogonb.exe: [-co] (Input)\n"
			"** Search for Orthogonal mates of Diagonal Latin Square **\n"
			" -c print number of transverses and exit\n"
			" -o output orthogonal mates like they are found\n"
			"Input: encoded diagonal latin square (see dlkconv.exe)\n"
			"Output: metadata and orthogonal squares in encoded format\n"
			"Returns 0 if orthogonal mates found, 1 if not.\n"
			"Example ortogonb.exe BJMzz45jMmo1FfZtCjN\n"
			"Author: Tomas Brada (GPL)\n";
		return 9;
	}
	try {
		bool orig_output = false;
		bool just_count = false;
		char* name = 0;
		if(string(argv[1])=="-c") {
			just_count= true;
			name= argv[2];
		} else
		if(string(argv[1])=="-o") {
			orig_output= true;
			name= argv[2];
		} else
			name= argv[1];
		Square in;
		if(!name && !name[0]) throw std::runtime_error("No input square");
		in.Decode(name);
		if(!in.width()) throw std::runtime_error("Zero-width input");

		Exact_cover_u dlx;
		if(just_count) {
			dlx.count_trans(in);
			std::cout<<"num_dtrans: "<<dlx.num_trans<<endl;
		} else {
			dlx.search_trans(in);
			std::cerr<<"num_dtrans: "<<dlx.num_trans<<endl;
			dlx.search_mates();
			std::cout<<"# in: "<<name<<endl;
			std::cout<<"# num_dtrans: "<<dlx.num_trans<<endl;
			std::cout<<"# num_mates: "<<(dlx.mates.size())<<endl;
			for(auto& m : dlx.mates) {
				if(orig_output)
					std::cout<<m<<endl;
				else
					std::cout<<m.Encode()<<endl;
			}
			std::cerr<<"num_mates: "<<(dlx.mates.size())<<endl;
			return !dlx.mates.size();
		}
	}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 3;
	}
	return 0;
}
