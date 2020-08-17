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

#include "dlk_util.cpp"
#include "exact_cover_mt.cpp"
#include "kanonizer_v.cpp"


/*
	.orto.todo : all co-squares
	.orto : all squares processed with DLX
*/

KanonizerV kanonizer;

int main(int argc, char* argv[])
{
	if(argc!=2) {
		std::cerr<<
			"ortowalk.exe: FileName\n"
			"** Explore the full graph of DLK orthogonality **\n"
			"Author: Tomas Brada (GPL)\n";
		return 9;
	}
	try {
		std::set<std::string> mates;
		std::set<const std::string*> todo;
		std::string file_name = argv[1];
		std::ofstream outputf(file_name, std::ios_base::app);
		std::ofstream todof(file_name+".todo", std::ios_base::app);
		{
			Square sq;
			size_t count = 0;
			std::ifstream todoin(file_name+".todo");
			while(todoin) {
				std::string line;
				std::getline(todoin,line);
				if( line!="" && line[0]!=' ' && line[0]!='#' ) {
					auto it = mates.insert(line);
					sq.Decode(line);
					it = mates.insert(kanonizer.Kanon(sq).Encode());
					todo.insert(&(*it.first));
					(std::cerr<<"\r"<<(++count)<<"    ").flush();
				}
			}
			std::ifstream fin(file_name);
			while(fin) {
				std::string line;
				std::getline(fin,line);
				if( line!="" && line[0]!=' ' && line[0]!='#' ) {
					mates.insert(line);
					sq.Decode(line);
					const auto it = mates.insert(kanonizer.Kanon(sq).Encode());
					todo.erase(&(*it.first));
					(std::cerr<<"\r"<<(++count)<<"    ").flush();
				}
			}
		}
		std::cerr<<endl;

		while(1) {
			const auto todo_it = todo.begin();
			if(todo_it==todo.end()) break;
			const std::string& current = **todo_it;
			std::cerr<<"in: "<<current<<" /"<<todo.size()<<endl;
			Square sq;
			sq.Decode(current);
			if(!sq.width()) throw std::runtime_error("Zero-width square");
			Exact_cover_u dlx;
			dlx.search_trans(sq);
			std::cerr<<"dtrans: "<<dlx.num_trans<<endl;
			if(dlx.num_trans>30000) {
				outputf<<"# todo dtrans: "<<dlx.num_trans<<" "<<current<<endl;
				todo.erase(todo_it);
				continue;
			}
			dlx.search_mates();
			std::cerr<<"mates: "<<dlx.mates.size()<<endl;
			size_t count = 0;
			for(auto& m : dlx.mates) {
				std::string enc = m.Encode();
				auto mate_it = mates.insert(enc);
				if(mate_it.second) { // unique
					todof<<enc<<endl;
					enc = kanonizer.Kanon(m).Encode();
					mate_it = mates.insert(enc);
					if(mate_it.second)
						todo.insert(&(*mate_it.first)); // kf is unique
					(std::cerr<<"\r"<<(++count)<<"    ").flush();
				}
			}
			todof.flush();
			outputf<<"# dtrans: "<<dlx.num_trans<<" mates: "<<dlx.mates.size()<<endl<<current<<endl;
			outputf.flush();
			todo.erase(todo_it);
		}
	}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 3;
	}
	return 0;
}
