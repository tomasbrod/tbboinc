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
#include "kanonizer_b.cpp"

Kanonizer kanonizer;

int main(int argc, char* argv[])
{
	if(argc!=1) {
		std::cerr<<
			"rules.exe: <input >output\n"
			"** Canonical Form Diagonal Latin Square Rule Extractor **\n"
			"Author: Tomas Brada (GPL)\n";
		return 9;
	}
	try {
		std::set<std::vector<unsigned>> ruleset;
		while(std::cin) {
			std::string line;
			std::getline(std::cin,line);
			if( line!="" && line[0]!=' ' && line[0]!='#' ) {
				Square sq;
				sq.Decode(line);
				if(!sq.width()) throw std::runtime_error("Zero-width square");

				std::set<Square> isotopes;
				kanonizer.Kanon(sq, &isotopes);
				std::vector<unsigned> map ( sq.width() );
				std::vector<unsigned> rule; rule.resize(sq.width(), sq.width());
				std::vector<unsigned> anti( sq.width() );
				const unsigned N = sq.width()-1;
				for(Square izo : isotopes) {
					for(unsigned i=0; i<sq.width(); ++i)
						map[izo(i,i)] = i;
					for(unsigned i=0; i<sq.width(); ++i)
						anti[i] = map[ izo(i, N-i) ];
					if(anti<rule)
						rule=anti;
				}
				auto it = ruleset.insert(rule);
				if(it.second) {
					for(unsigned i=0; i<N; ++i)
						std::cout<<rule[i]<<" ";
					std::cout<<rule[N]<<endl;
					std::cout.flush();
				}
			}
		}
		std::cout<<"Rules: "<<ruleset.size()<<endl;
	}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 1;
	}
	return 0;
}
