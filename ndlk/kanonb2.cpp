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
#include "kanonizer_v.cpp"

Kanonizer kanonizerB;
KanonizerV kanonizerV;

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

				/*
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
				std::cout<<"#cnt_izo: "<<isotopes.size()<<endl<<min<<min.Encode()<<endl;
				*/

				kanonizerV.init_order(sq.width());
				std::cout<<"# im_mtrans.size()= "<<kanonizerV.im_mtrans.size()<<"  im_tindex.size()= "<<kanonizerV.im_tindex.size()<<"  im_diagonals.size()= "<<kanonizerV.im_diagonals.size()<<endl;
				Square min2 = kanonizerV.Kanon(sq);
				std::cout<<endl<<min2<<min2.Encode()<<endl;
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
