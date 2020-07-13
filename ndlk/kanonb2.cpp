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

void Kanon_test(Square& min, const Square& sq)
{
	if(sq<min)
		min= sq;
}

void Kanon_last(Square& min, Square& sq)
{
	const unsigned n = sq.width();
	const unsigned N = sq.width()-1;
	Square trsp(n), rot1(n), rot2(n), rot3(n), vert(n), horz(n), anti(n);
	for(unsigned i=0; i<n; ++i)
		for(unsigned j=0; j<n; ++j) {
			trsp(i,j) = sq(j,i);
			horz(i,j) = sq(i, N-j);
			vert(i,j) = sq(N-i, j);
			rot2(i,j) = sq(N-i, N-j);
			rot1(i,j) = sq(N-j, i);
			rot3(i,j) = sq(j, N-i);
			anti(i,j) = sq(N-j, N-i);
	}
	Kanon_test(min,trsp.DiagNorm());
	Kanon_test(min,rot1.DiagNorm());
	Kanon_test(min,rot2.DiagNorm());
	Kanon_test(min,rot3.DiagNorm());
	Kanon_test(min,vert.DiagNorm());
	Kanon_test(min,horz.DiagNorm());
	Kanon_test(min,anti.DiagNorm());
	Kanon_test(min,sq.DiagNorm());
}

void Kanon_M1(Square& sq, unsigned I)
{
	const unsigned N = sq.width()-1;
	for(unsigned r=0; r<=N; ++r) {
		std::swap( sq(r,I), sq(r,N-I) );
	}
	for(unsigned c=0; c<=N; ++c) {
		std::swap( sq(I,c), sq(N-I,c) );
	}
}

void Kanon_M2(Square& sq, unsigned I, unsigned J)
{
	const unsigned N = sq.width()-1;
	for(unsigned r=0; r<=N; ++r) {
		std::swap( sq(r,I), sq(r,J) );
		std::swap( sq(r,N-J), sq(r,N-I) );
	}
	for(unsigned c=0; c<=N; ++c) {
		std::swap( sq(I,c), sq(J,c) );
		std::swap( sq(N-J,c), sq(N-I,c) );
	}
}

void Kanon_Apply(Square& sq, std::pair<int,int> p)
{
	if(p.first==-1) {
		Kanon_M1(sq, p.second);
	} else {
		Kanon_M2(sq, p.first, p.second);
	}
}
	
Square Kanon(Square sq)
{
	std::vector<std::pair<int,int>> optX;
	std::vector<bool> opts;
	std::vector<unsigned> stack;
	for(unsigned i=0; i< sq.width()/2; ++i) {
		optX.push_back(std::pair<int,int>{-1,i});
		opts.push_back(0);
	}
	for(unsigned i=0; i< sq.width()/2; ++i)
	for(unsigned j=i+1; j< sq.width()/2; ++j) {
		optX.push_back(std::pair<int,int>{i,j});
		opts.push_back(0);
	}
	Square min(sq.DiagNorm());
	unsigned i = 0, p = 0;
	stack.resize(optX.size()+1);
	//std::cerr<<"optX.size(): "<<optX.size()<<endl;

	while(1) {
		for(; i<optX.size() && opts[i]; ++i) {}
		if(i >= optX.size()) {
			if(!p) break;
			i=stack[--p];
			opts[i]=0;
			//std::cerr<<"U "<<i<<endl;
			Kanon_Apply(sq, optX[i]);
			i++;
		} else {
			//std::cerr<<"A "<<i<<endl;
			Kanon_last(min, sq);
			Kanon_Apply(sq, optX[i]);
			opts[i]=1;
			stack[p++] = i;
		}
	}

	return min;
}

void Kanonize(Square sq)
{
	Exact_cover_u dlx;
	//std::cout<<"orig"<<endl<<sq;
	//dlx.count_trans(sq);
	//std::cout<<"dtrans: "<<dlx.num_trans<<endl;
	Square min = Kanon(sq);
	//std::cout<<"min"<<endl<<min;
	//dlx.count_trans(min);
	//std::cout<<"dtrans: "<<dlx.num_trans<<endl;
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
	try {
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
	}
	catch( const std::exception& e ) {
		std::cerr<<"Exception: "<<e.what()<<endl;
		return 1;
	}
	return 0;
}
