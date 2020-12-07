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
#include "kanonizer_v.cpp"

KanonizerV kanonizer;

void fill(unsigned order)
{
	//const unsigned order = 12;
	kanonizer.init_order(order);
	Square sq(order);
	std::set<std::vector<unsigned>> ruleset;
	std::vector<unsigned> rule ( order, order );
	//natural main diagonal
	for(unsigned i=0; i<order; ++i)	sq(i,i) = i;
	std::vector<bool> options(order,false);
	//for odd-order, fix the middle cell
	if(order&1) options[order/2] = true;
	unsigned r=0; // row
	unsigned v=0; // value
	unsigned count = 0;
	while(1) {
		if(r<order) {
			for(1; v<order && (options[v] || v==sq(r,r) || v==sq(order-r-1,order-r-1)); ++v) ;
			if(v<order) {
				//std::cout<<r<<"+"<<v<<endl;
				sq.anti(r) = v;
				options[v] = true;
				r++;
				if(order&1 && r==order/2) r++;
				v=0;
				continue;
			}
		} else {
			//std::cout<<sq<<endl;
			kanonizer.im_find_can(0, &rule[0], sq);
			/*for(unsigned i=0; i<order; ++i)
				std::cout<<rule[i]<<" ";
			// 0 2 1 4 3 6 5 8 7 9 is wrong, 0 cant be first!!!
			std::cout<<endl;*/
			auto it = ruleset.insert(rule);
			if(it.second) {
				for(unsigned i=0; i<(order-1); ++i)
						std::cout<<rule[i]<<" ";
					std::cout<<rule[order-1]<<endl;
			}
			count++;
			if(!(count&127)) {
				std::cerr<<"\rFillings: "<<count<<" ,rules: "<<ruleset.size()<<"  @";
				for(unsigned i=0; i<(order-1); ++i)
						std::cerr<<" "<<sq.anti(i);
				(std::cerr<<"    ").flush();
			}
		}
		if(!r) break;
		r--;
		if(order&1 && r==order/2) r--;
		v=sq.anti(r);
		//std::cout<<r<<"-"<<v<<endl;
		options[v] = false;
		v++;
	}
	std::cout<<"\n# Fillings: "<<count<<" ,rules: "<<ruleset.size()<<endl;
}

int main(int argc, char* argv[])
{
	if(argc!=2) {
		std::cerr<<
			"rules.exe: N >output\n"
			"** Canonical Form Diagonal Latin Square Rule Extractor **\n"
			"Author: Tomas Brada (GPL)\n";
		return 9;
	}
	try {
		fill(atoi(argv[1]));
	}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 1;
	}
	return 0;
}
