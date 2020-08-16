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

	Square last_sq[8];
	std::vector<std::pair<int,int>> optX;
	unsigned order=0;
	
	bool Last(Square& min, Square& sq, std::set<Square> *SOS)
	{
		const unsigned n = sq.width();
		const unsigned N = sq.width()-1;
		bool dupe = false;
		Square *lsq = last_sq;
		for(unsigned i=0, o=0; i<n; ++i) {
			for(unsigned j=0; j<n; ++j, ++o) {
				lsq[0][o] = sq(i,j);
				lsq[1][o] = sq(N-i, j);
				lsq[2][o] = sq(N-i, N-j);
				lsq[3][o] = sq(j,i);
				lsq[4][o] = sq(j, N-i);
				lsq[5][o] = sq(N-j, i);
				lsq[6][o] = sq(N-j, N-i);
				lsq[7][o] = sq(i, N-j);
			}
		}
		for(unsigned k=0; k<8; ++k) {
			//lsq[k].DiagNorm();
			if(SOS) {
				auto it = SOS->insert(lsq[k]);
				if(!it.second) dupe=true;
			}
			if(lsq[k]<min)
				min= lsq[k];
		}
		return !dupe;
	}

	void ApplyM(Square& sq, std::pair<int,int> p)
	{
		const unsigned N = sq.width()-1;
		const unsigned J = p.first;
		const unsigned I = p.second;
		if(p.first==-1) {
			for(unsigned r=0; r<=N; ++r) {
				std::swap( sq(r,I), sq(r,N-I) );
			}
			for(unsigned c=0; c<=N; ++c) {
				std::swap( sq(I,c), sq(N-I,c) );
			}
		} else {
			for(unsigned r=0; r<=N; ++r) {
				std::swap( sq(r,I), sq(r,J) );
				std::swap( sq(r,N-J), sq(r,N-I) );
			}
			for(unsigned c=0; c<=N; ++c) {
				std::swap( sq(I,c), sq(J,c) );
				std::swap( sq(N-J,c), sq(N-I,c) );
			}
		}
	}

	Square Kanon(Square sq, std::set<Square> *SOS = nullptr)
	{
		std::vector<bool> opts;
		std::vector<unsigned> stack;
		if(sq.width()!=order) {
			order = sq.width();
			for(unsigned i=0; i< sq.width()/2; ++i) {
				optX.push_back(std::pair<int,int>{-1,i});
			}
			for(unsigned i=0; i< sq.width()/2; ++i)
			for(unsigned j=i+1; j< sq.width()/2; ++j) {
				optX.push_back(std::pair<int,int>{i,j});
			}
			for(int i=0; i<8; ++i)
				last_sq[i]=Square(sq.width());
			std::cout<<"#optX.size(): "<<optX.size()<<endl;
		}
		Square min(sq);
		unsigned i = 0, p = 0;
		stack.resize(optX.size()+1);
		opts.resize(optX.size(),0);

		//Last(min, sq, SOS);

		while(1) {
			for(; i<optX.size() && opts[i]; ++i) {}
			if(i >= optX.size()) {
				if(!p) break;
				i=stack[--p];
				opts[i]=0;
				//std::cerr<<"U "<<i<<endl;
				ApplyM(sq, optX[i]);
				i++;
			} else {
				//std::cerr<<"A "<<i<<endl;
				ApplyM(sq, optX[i]);
				Last(min, sq, SOS);
				stack[p++] = i;
				opts[i]=1;
			}
		}

		return min;
	}


std::vector<std::pair<int,int>> im_mtrans; // list of valid m-transform combinations
std::vector<uint32_t> im_tindex; // transformation indices
std::vector<uint8_t> im_diagonals;  // indices of diagonals (max 15*15)

void im_get_rule(unsigned *rule, const Square& sq, unsigned index)
{
	unsigned nmap[ sq.width() ];
	const auto *mdiag = &im_diagonals[ (index*2+0)*sq.width() ];
	const auto *adiag = &im_diagonals[ (index*2+1)*sq.width() ];
	for(unsigned i=0; i<sq.width(); ++i)
		nmap[ sq[mdiag[i]] ] = i;
	for(unsigned i=0; i<sq.width(); ++i)
		rule[i] = nmap[ sq[adiag[i]] ];
}
		
		

void transform_diag(unsigned *iso, unsigned n)
{
	const unsigned N = n - 1;
	for(unsigned i=0; i<n; ++i) {
				//lsq[0][o] = sq(i,j); 0
				//iso[0*n+i] = iso[i];
				//iso[1*n+i] = iso[i+n];
				//lsq[1][o] = sq(N-i, j); 1
		iso[2*n+i] = iso[N-i+n];
		iso[3*n+i] = iso[N-i];
				//lsq[2][o] = sq(N-i, N-j); 2
		iso[4*n+i] = iso[N-i];
		iso[5*n+i] = iso[N-i+n];
				//lsq[3][o] = sq(j,i); 3
		iso[6*n+i] = iso[i];
		iso[7*n+i] = iso[N-i+n];
				//lsq[4][o] = sq(j, N-i); 4
		iso[8*n+i] = iso[N-i+n];
		iso[9*n+i] = iso[i];
				//lsq[5][o] = sq(N-j, i); 5
		iso[10*n+i] = iso[i+n];
		iso[11*n+i] = iso[N-i];
				//lsq[6][o] = sq(N-j, N-i); 6
		iso[12*n+i] = iso[N-i];
		iso[13*n+i] = iso[i+n];
				//lsq[7][o] = sq(i, N-j); 7
		iso[14*n+i] = iso[i+n];
		iso[15*n+i] = iso[i];
	}
}

// find the canonical diagonal form
void im_find_can(unsigned *rule, unsigned &m_index, unsigned &t_index, const Square& sq)
{
	const unsigned order = sq.width();
	unsigned tdiag[order*2*8];
	unsigned nmap[order];
	for(unsigned i=0; i<order; ++i) rule[i]=order;
	for(unsigned m=0; m < im_tindex.size(); ++m) {
		for(unsigned i=0; i<(order*2); ++i) {
			unsigned d = im_diagonals[m*order*2+i];
			tdiag[i] = sq [ d ];
		}
		transform_diag(tdiag, order);
		for(unsigned t=0; t<8; ++t) {
			unsigned* diag = &tdiag[order*2*t];
			for(unsigned i=0; i<order; ++i) {
				nmap[diag[i]] = i;
			}
			for(unsigned i=order; i<(order*2); ++i) {
				unsigned  v = diag[i] = nmap[ diag[i] ];
				if( v > rule[i-order] ) goto larger;
			}
			m_index = m;
			t_index = t;
			std::copy(diag+order, diag+order+order, rule);
			larger:;
		}
	}
}

Square transform_sq(const Square& sq, unsigned x)
{
	const unsigned n = sq.width();
	const unsigned N = sq.width()-1;
	Square lsq(n);
	for(unsigned i=0, o=0; i<n; ++i) {
		for(unsigned j=0; j<n; ++j, ++o) switch(x) {
			case 0:
				lsq[o] = sq(i,j);
			break; case 1:
				lsq[o] = sq(N-i, j);
			break; case 2:
				lsq[o] = sq(N-i, N-j);
			break; case 3:
				lsq[o] = sq(j,i);
			break; case 4:
				lsq[o] = sq(j, N-i);
			break; case 5:
				lsq[o] = sq(N-j, i);
			break; case 6:
				lsq[o] = sq(N-j, N-i);
			break; case 7:
				lsq[o] = sq(i, N-j);
			break;
		}
	}
	return lsq;
}

Square im_get_can(Square sq)
{
	const unsigned order = sq.width();
	const unsigned N = sq.width()-1;
	unsigned rule[order];
	unsigned m_index, t_index;
	im_find_can(rule, m_index, t_index, sq);
					for(unsigned i=0; i<N; ++i)
						std::cout<<rule[i]<<" ";
					std::cout<<rule[N]<<endl;
	for(unsigned i=0; i<=im_mtrans.size(); ++i) {
		if( (1<<i) & im_tindex[m_index] ) {
			ApplyM(sq, im_mtrans[i]);
	}}
	transform_sq(sq, t_index);
	sq.DiagNorm();
	return sq;
}

void init_order(unsigned order)
{
	const unsigned N = order-1;
	im_mtrans.clear();
	// it works same with i=1 here...
	for(unsigned i=0; i< order/2; ++i) {
		im_mtrans.push_back(std::pair<int,int>{-1,i});
	}
	for(unsigned i=0; i< order/2; ++i)
		for(unsigned j=i+1; j< order/2; ++j) {
			im_mtrans.push_back(std::pair<int,int>{i,j});
	}
	Square natural(order);
	for(unsigned i=0; i<natural.size(); ++i)
		natural[i] = i;
	std::set<std::vector<uint8_t>> diag_izotopes;

	std::array<Square,8> lsq;
	for(int i=0; i<8; ++i)
		lsq[i]=Square(order);
	std::vector<bool> opts(im_mtrans.size(),0);
	std::vector<unsigned> stack(im_mtrans.size()+1);
	unsigned transformation_index = 0;

	#if 0
	for(unsigned i=0; i<order; ++i)
		diag_izotope[i] = natural(i,i);
	for(unsigned i=0; i<order; ++i)
		diag_izotope[order+i] = natural(i,N-i);
	diag_izotopes.insert(diag_izotope);
	im_tindex.push_back( transformation_index );
	im_diagonals.insert( im_diagonals.end(), diag_izotope.begin(), diag_izotope.end() );

	unsigned i = 0, p = 0;
	while(1) {
		for(; i<im_mtrans.size() && opts[i]; ++i) ;
		if(i >= im_mtrans.size()) {
			if(!p) break;
			i=stack[--p];
			opts[i]=0;
			ApplyM(natural, im_mtrans[i]);
			transformation_index ^= 1<<i;
			i++;
		} else {
			ApplyM(natural, im_mtrans[i]);
			transformation_index ^= 1<<i;
			//
			for(unsigned i=0, o=0; i<order; ++i) {
				for(unsigned j=0; j<order; ++j, ++o) {
					lsq[0][o] = natural(i, N-j);
					lsq[1][o] = natural(N-i, j);
					lsq[2][o] = natural(N-i, N-j);
					lsq[3][o] = natural(j,i);
					lsq[4][o] = natural(j, N-i);
					lsq[5][o] = natural(N-j, i);
					lsq[6][o] = natural(N-j, N-i);
					lsq[7][o] = natural(i,j);
				}
			}
			bool uniq=true;
			for(unsigned k=0; k<8 && uniq; ++k) {
				for(unsigned i=0; i<order; ++i)
					diag_izotope[i] = lsq[k](i,i);
				for(unsigned i=0; i<order; ++i)
					diag_izotope[order+i] = lsq[k](i,N-i);
				auto it = diag_izotopes.insert(diag_izotope);
				uniq= uniq && it.second;
			}
			if(uniq) {
				im_tindex.push_back( transformation_index );
				im_diagonals.insert( im_diagonals.end(), diag_izotope.begin(), diag_izotope.end() );
			}
			//
			stack[p++] = i;
			opts[i]=1;
		}
	}
	#else
	//transformation 0 appears to have no effect, maybe we do not have to test without it?
	for(transformation_index=0; transformation_index < (1<<im_mtrans.size()); transformation_index+=1) {
		Square sq(natural);
		for(unsigned i=0; i<=im_mtrans.size(); ++i) {
			if( (1<<i) & transformation_index ) {
				ApplyM(sq, im_mtrans[i]);
		}}
		unsigned cur_iso[order*2*8];
		for(unsigned i=0; i<order; ++i)
			cur_iso[i] = sq(i,i);
		for(unsigned i=0; i<order; ++i)
			cur_iso[order+i] = sq(i,N-i);
		transform_diag(cur_iso,order);

		bool uniq=true;
		for(unsigned k=0; k<8 && uniq; ++k) {
			std::vector<uint8_t> diag_izotope(cur_iso+(k*order*2),cur_iso+((k+1)*order*2));
			auto it = diag_izotopes.insert(diag_izotope);
			uniq= uniq && it.second;
		}
		if(uniq) {
			im_tindex.push_back( transformation_index );
			im_diagonals.insert( im_diagonals.end(), cur_iso, cur_iso+(order*2) );
		}
	}
	#endif
}


// order 11 - 262144 (2^15*8) total transformations, 15360 unique ones (17x)
// order 12 - (2^21*8), 184320 unique, 24 bit tr
// order 13 - (2^21*8), 184320 unique, 24 bit tr
// order 15 - (2^  *8), 
// only the x is important
// store both diagonals and transorm(18 bits)
// find the minimal rule, foreach:
// buld normalization map, normalize antidiag
// opt works 11, 12  ... vymenit, otocit jednu, druhu

void izo_cmd()
{
	Square sq(11);
	for(unsigned i=0; i<sq.size(); ++i)
		sq[i] = i+100;
	for(unsigned i=0; i<sq.width(); ++i)
		sq(i,i) = i;
	unsigned rule[11] = {1, 0, 3, 2, 7, 5, 8, 4, 6, 10, 9};
	/*std::cout<<"#before"<<endl<<sq<<endl;

	Square min;
	std::set<Square> isotopes;
	min= Kanon(sq,&isotopes);
	unsigned cizo = isotopes.size();
	std::cout<<"#cnt_izo: "<<cizo<<endl;*/

	init_order(12);
	std::cout<<"# im_mtrans.size()= "<<im_mtrans.size()<<"  im_tindex.size()= "<<im_tindex.size()<<"  im_diagonals.size()= "<<im_diagonals.size()<<endl;

	/*for(const auto& izo : im_tindex) {
		std::cout<<izo<<endl;
	}
	for(const auto& izo : isotopes) {
		std::cout<<izo<<endl;
	}*/
}

void kanon_cmd()
{
			while(std::cin) {
			std::string line;
			std::getline(std::cin,line);
			if( line!="" && line[0]!=' ' && line[0]!='#' ) {
				Square sq;
				Square min;
				sq.Decode(line);
				if(!sq.width()) throw std::runtime_error("Zero-width square");
				sq.DiagNorm();

				std::set<Square> isotopes;
				Kanon(sq, &isotopes);
				std::vector<unsigned> map ( sq.width() );
				std::vector<unsigned> rule; rule.resize(sq.width(), sq.width());
				std::vector<unsigned> anti( sq.width() );
				const unsigned N = sq.width()-1;
				for(Square izo : isotopes) {
					izo.DiagNorm();
					for(unsigned i=0; i<sq.width(); ++i)
						map[izo(i,i)] = i;
					for(unsigned i=0; i<sq.width(); ++i)
						anti[i] = map[ izo(i, N-i) ];
					if(anti<rule)
						rule=anti;
						min=izo;
				}

				
				std::cout<<min.Encode()<<endl;
				init_order(sq.width());
				std::cout<<"# im_mtrans.size()= "<<im_mtrans.size()<<"  im_tindex.size()= "<<im_tindex.size()<<"  im_diagonals.size()= "<<im_diagonals.size()<<endl;
				Square min2 = im_get_can(sq);
				if(min!=min2) {
					std::cout<<"mismatch, "<<min2.Encode()<<endl;
				}
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
