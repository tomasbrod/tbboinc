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

bool isOrthogonal(const Square& sqa, const Square& sqb)
{
	const unsigned n = sqa.width();
	if(sqb.width()!=n) return false;
	bool b[sqa.size()] = {0};
	for(size_t i=0; i<sqa.size(); ++i) {
		unsigned u=sqa[i]*n+sqb[i];
		if(b[u])
			return false;
		b[u]=1;
	}
	return true;
}

std::vector<Square*> FindMates(const Square& sq, std::vector<Square>& squares)
{
	std::vector<Square*> r;
	for( auto& m : squares )
		if(isOrthogonal(sq, m))
			r.push_back( &m );
	return r;
}

int main(int argc, char* argv[])
{
	if(argc!=1) {
		std::cerr<<
			"isortogon.exe: <Input >Output\n"
			"** Check Diagonal Latin Squares for Orthogonality **\n"
			"In the input set, checks if any LK are orthogonal to themselves and each other.\n";
		return 9;
	}
	/* Read input into vector (unpacked?). While doing that, check for self-orthogonality.
	   Then check each pair for orthogonality.
	*/
	//try {
		std::vector<Square> squares;
		while(std::cin) {
			string line;
			getline(std::cin, line);
			if(line.empty() || line[0]=='#') continue;
			Square sq;
			sq.Decode(line);
			if(!sq.width()) continue;
			bool self = isOrthogonal(sq,sq);
			auto mates = FindMates(sq,squares);
			if( self || mates.size() ) {
				std::cout<<line<<endl;
				std::cout<<"# orthogonal "<<mates.size()<< (self? " Self" : " N") <<endl;
				if(mates.size()) {
					for( auto m : mates )
						std::cout<<"#-"<<m->Encode()<<endl;
				}
			}
			squares.push_back(sq);
		}
	/*}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 1;
	}*/
	return 0;
}
