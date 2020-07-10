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

#define DLX_QUIET

#include "dlk_util.cpp"
#include "exact_cover_mt.cpp"
#include "kanonizer_b.cpp"

Kanonizer kanonizer;

void Kanonize(Square sq)
{
	/*Exact_cover_u dlx;
	std::cout<<"orig"<<endl<<sq;
	dlx.count_trans(sq);
	std::cout<<"dtrans: "<<dlx.num_trans<<endl;
	Square min = Kanon(sq);
	std::cout<<"min"<<endl<<min;
	dlx.count_trans(min);
	std::cout<<"dtrans: "<<dlx.num_trans<<endl;
	std::cout<<min.Encode()<<endl;*/
	Square min = kanonizer.Kanon(sq);
	//std::cout<<"min"<<endl<<min;
	std::cout<<min.Encode()<<endl;
}

int main(int argc, char* argv[])
{
	if(argc!=1) {
		std::cerr<<
			"kanonb.exe: <input >output\n"
			"** Diagonal Latin Square Kanonizer **\n"
			"Author: Tomas Brada (GPL)\n";
		return 9;
	}
	//try {
		while(std::cin) {
			std::string line;
			std::getline(std::cin,line);
			if( line!="" && line[0]!=' ' && line[0]!='#' ) {
				Square sq;
				sq.Decode(line);
				if(!sq.width()) throw std::runtime_error("Zero-width square");
				Kanonize(sq);
			}
		}
	/*}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 1;
	}*/
	return 0;
}
