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
using std::ios;

#define DLX_QUIET

#include "dlk_util.cpp"
#include "kanonizer_b.cpp"
#include "kanonizer_v.cpp"

KanonizerV kanonizerV;

Kanonizer kanonizerB;

Square get_diag_kanon_b(Square sq)
{
	std::set<Square> isotopes;
	kanonizerB.Kanon(sq, &isotopes);
	std::vector<unsigned> rule; rule.resize(sq.width(), sq.width());
	std::vector<unsigned> anti( sq.width() );
	const unsigned N = sq.width()-1;
	Square min;
	for(Square izo : isotopes) {
		izo=izo.DiagNorm();
		for(unsigned i=0; i<sq.width(); ++i)
			anti[i] = izo(i, N-i);
		if(anti<rule) {
			rule=anti;
			min=izo;
		}
		else if(anti==rule && izo<min) {
			min=izo;
		}
	}
	return min;
}

void kanon_cmd()
{
			while(std::cin) {
			std::string line;
			std::getline(std::cin,line);
			if( line!="" && line[0]!=' ' && line[0]!='#' ) {
				Square sq;
				sq.Decode(line);
				if(!sq.width()) throw std::runtime_error("Zero-width square");

				sq.DiagNorm();
				Square min2 = kanonizerV.Kanon(sq);
				std::cout<<min2.Encode()<<endl;
			}
		}
}

int main(int argc, char* argv[])
{
	if(argc!=1) {
		std::cerr<<
			"kanonb.exe: <input >output\n"
			"** Diagonal Latin Square XXX **\n"
			"Author: Tomas Brada (GPL)\n";
		return 9;
	}
	try {
		kanon_cmd();
	}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 1;
	}
	return 0;
}
