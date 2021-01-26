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

struct Exact_cover_2 : Exact_cover_u {
	typedef Exact_cover_u::nodeix nodeix;
	void search_trans(const Square& dlk)
	{
		init_trans(dlk);
		dance(&Exact_cover_2::save_trans);
	}
	void search_mates_cout()
	{
		init_disjoint();
		bool found = 0;
		do {
			dance(&headsDyn[0],&nodesDyn[0],&O2[0],&found,order);
			if(found) {
				Square mate(order);
				get_mate(mate,&O2[0]);
				std::cout<<mate.Encode()<<endl;
				std::cout.flush();
			} else break;
		} while(!stop_dance);
	}
	int search_mates_cout(const nodeix* path2)
	{
		init_disjoint();
		int level=0;
		if(path2) {
			for(const nodeix* path=path2; *path; ++path) {
				nodeix c = choose_column(headsDyn.data());
				cover_column(c, headsDyn.data(),nodesDyn.data());
				nodesDyn[nodesDyn[c].up].down = 0;
				nodeix r=c;
				std::cerr<<"L("<<level<<") c("<<c<<") "<<*path<<" / "<<headsDyn[c].size<<endl;
				//if(*path>headsDyn[c].size) return -1;
				for( unsigned i=0; i<*path; ++i) {
					r = nodesDyn[r].down;
					if(!r) return -1;
				}
				for(nodeix j = nodesSt[r].right; j != r; j = nodesSt[j].right)
					cover_column(nodesSt[j].column, headsDyn.data(),nodesDyn.data());
				O2[level] = r;
				//std::cerr<<"r="<<r<<endl;
				level++;
			}
		}
		{
			nodeix c = choose_column(headsDyn.data());
			std::cerr<<"L("<<level<<") c("<<c<<") X / "<<headsDyn[c].size<<endl;
		}
		bool found = 0;
		do {
			dance(&headsDyn[0],&nodesDyn[0],&O2[level],&found,order-level);
			if(found) {
				Square mate(order);
				get_mate(mate,&O2[0]);
				//std::cout<<"#";	for(unsigned i=0; i<order; ++i) std::cout<<" "<<O2[i]; std::cout<<endl;
				std::cout<<mate.Encode()<<endl;
				std::cout.flush();
			} else break;
		} while(!stop_dance);
		return level;
	}

			
};

int main(int argc, char* argv[])
{
	if(argc<=1) {
		std::cerr<<
			"ortogonbw.exe: (Input) (Path)\n"
			"** Search for Orthogonal mates of Diagonal Latin Square **\n"
			"Input: encoded diagonal latin square (see dlkconv.exe)\n"
			"Output: metadata and orthogonal squares in encoded format\n"
			"Prints to output immediately as square is found. Single thread.\n"
			"Path: sequence of space-separated numbers narrowing the problem space\n"
			"Explanation: L(level) c(column) choosen-row / count-rows\n"
			"Specify number as Path from 1 up to count-rows to narrow down the Search,\n"
			"specify multiple numbers to narrow down further.\n"
			"Author: Tomas Brada (GPL)\n";
		return 9;
	}
	try {
		char* name = 0;
		name= argv[1];
		std::vector<Exact_cover_2::nodeix> path;
		path.resize(argc-1);
		path[argc-2]=0;
		Square in;
		if(!name && !name[0]) throw std::runtime_error("No input square");
		in.Decode(name);
		if(!in.width()) throw std::runtime_error("Zero-width input");

		Exact_cover_2 dlx;
		dlx.search_trans(in);
		std::cerr<<"num_dtrans: "<<dlx.num_trans<<endl;
		std::cout<<"# in: "<<name;
		for(unsigned i=2; i<argc; ++i) {
			path[i-2]=atoi(argv[i]);
			std::cout<<" "<<argv[i];
		}
		std::cout<<endl<<"# num_dtrans: "<<dlx.num_trans<<endl;
		dlx.search_mates_cout(path.data());
		//std::cout<<"# num_mates: "<<(dlx.mates.size())<<endl;
		return 0;
	}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 3;
	}
	return 0;
}
