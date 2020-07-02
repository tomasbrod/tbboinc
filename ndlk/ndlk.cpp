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
/* arbitrary-size DLK */

/* enumerate transversals, dtrans, disjoint trans */
/* ortogon_u can build ODLK of 12, maybe others? */

#include "dlk_util.cpp"
#include "exact_cover_mt.cpp"


void test_mul() {
	std::vector<unsigned short> v;
	v.push_back(0);
	mulBc( 123, 1, v, 10);
	for(const auto V : v) std::cout<<V; std::cout<<endl;
	mulBc( 2, 1, v, 10);
	for(const auto V : v) std::cout<<V; std::cout<<endl;
	mulBc( 0, 2, v, 10);
	for(const auto V : v) std::cout<<V; std::cout<<endl;
	mulBc( 4, 2, v, 10);
	for(const auto V : v) std::cout<<V; std::cout<<endl;
	mulBc( 4, 0, v, 10);
	for(const auto V : v) std::cout<<V; std::cout<<endl;
}

void test_div() {
	std::vector<unsigned short> v;
	v.push_back(0);
	mulBc( 1234, 1, v, 10);
	for(const auto V : v) std::cout<<V; std::cout<<endl;
	std::cout<< divB(2,v,10) <<endl;
	for(const auto V : v) std::cout<<V; std::cout<<endl;
	std::cout<< divB(11,v,10) <<endl;
	for(const auto V : v) std::cout<<V; std::cout<<endl;
}

struct InputSpec {
	std::string name;
	std::istream* is;
	std::unique_ptr<std::ifstream> ifstr;
	bool enc, alnum;
	char** init(char **arg)
	{
		enc=alnum=0;
		name.clear(); is=0; ifstr.reset();
		if(!arg[0]) throw std::runtime_error("Missing input specifier");
		int source=0; // imm, stdin, file
		for(unsigned i=0; arg[0][i]; ++i) {
			char c = arg[0][i];
			if(c=='e')
				enc=1;
			else if(c=='a')
				alnum=1;
			else if(c=='s')
				source=1;
			else if(c=='f')
				source=2;
			else throw std::runtime_error("Invalid input specifier: "+c);
		}
		arg++;
		if(enc && alnum) throw std::runtime_error("Encoded (e) and Alphanumeric (a) input may not be combined");
		if(!enc && !source) throw std::runtime_error("Alphanumeric (a) or Numeric () input requites Stdin (s) of File (f)");
		if(2==source) {
			name= std::string(*(arg++));
			ifstr.reset( new std::ifstream(name) );
			if(!*ifstr) throw std::runtime_error("Input open error");
			is = ifstr.get();
		} else
		if(1==source) {
			is = &std::cin;
		} else {
			is=0;
			name=std::string(*(arg++));
		}
		return arg;
	}
	Square get() {
		Square sq;
		if(is && !(*is)) return sq;
		if(enc) {
			if(is) {
				std::string line;
				getline(*is,line);
				sq.Decode(line);
			} else {
				if(name.empty()) return sq;
				sq.Decode(name);
				name.clear();
		}}
		else if(alnum) {
			sq.ReadAlnum(*is);
			sq.Normaliz();
		} else {
			sq.Read(*is);
			sq.Normaliz();
		}
		return sq;
	}
};

struct OutputSpec {
	std::string name;
	std::ostream* os;
	std::unique_ptr<std::ofstream> ofstr;
	bool enc, alnum, compact, diag;
	char** init(char **arg)
	{
		enc=alnum=compact=diag=0;
		name.clear(); os=&std::cout; ofstr.reset();
		if(!arg[0]) return arg; // this is ok
		int source=1; // imm, stdout, file
		for(unsigned i=0; arg[0][i]; ++i) {
			char c = arg[0][i];
			if(c=='e')
				enc=1;
			else if(c=='a')
				alnum=1;
			else if(c=='c')
				compact=1;
			else if(c=='d')
				diag=1;
			else if(c=='s')
				source=1;
			else if(c=='f')
				source=2;
			else throw std::runtime_error("Invalid input specifier: "+c);
		}
		arg++;
		if(enc && alnum) throw std::runtime_error("Encoded (e) and Alphanumeric (a) input may not be combined");
		//... more checks assert(source);
		if(2==source) {
			name= std::string(*(arg++));
			ofstr.reset( new std::ofstream(name) );
			if(!*ofstr) throw std::runtime_error("Output open error");
			os = ofstr.get();
		}
		return arg;
	}
	void put(Square sq) {
		if(enc) {
			(*os)<< sq.Encode() << endl;
		}
		else {
			if(diag)
				sq.DiagNorm();
			else sq.Normaliz();
			if(alnum) 
				sq.WriteAlnum(*os);
			else
				(*os)<<sq;
			if(!compact) (*os)<<endl;
		}
	}
};

int main(int argc, char* argv[])
{
	char** arg = argv+1;
	InputSpec in;
	OutputSpec out;
	arg= in.init(arg);
	arg= out.init(arg);
	while(1) {
		Square sq = in.get();
		if(!sq.width()) break;
		out.put(sq);
	}
	return 0;

#if 0
	Square sq;
	sq.ReadAlnum(std::cin);
	std::cerr<<"width: "<<sq.width()<<endl;
	sq.Normaliz();
	std::cout<<sq<<endl;
	return 8;

	std::cerr<<sq<<endl;
	std::string enc = sq.Encode();
	std::cerr<<enc<<endl;
	Square sq2; sq2.Decode(enc);
	if(sq==sq2)
		std::cerr<<"match"<<endl;
	else
		std::cerr<<sq2<<endl;
	
	std::cerr<<"something else"<<endl;
	sq2.Decode("58");
	std::cerr<<sq2<<endl;
	
	return 6;
	//sq= sq.Transpose().SwapYV();
	std::cout<<"isLK: "<<sq.isLK()<<endl;
	std::cout<<"isDLK: "<<sq.isDLK()<<endl;
	sq.Normaliz();
	std::cout<<sq;
	Exact_cover_u dlx;
	//dlx.search_trans(sq);
	dlx.count_trans(sq);
	std::cerr<<"num_trans: "<<dlx.num_trans<<endl;
	std::cout<<"num_trans: "<<dlx.num_trans<<endl;
	return 0;
	dlx.search_mates();
	for(auto& m : dlx.mates)
		std::cout<<m<<endl;
	std::cerr<<"num_mates: "<<(dlx.mates.size())<<endl;
	return 69;
#endif
}
