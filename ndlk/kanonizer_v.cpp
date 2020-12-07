#include <cassert>
struct KanonizerV {
	unsigned order=0;
	std::vector<std::pair<int,int>> im_mtrans;
	std::vector<uint64_t> im_isotopes;
	std::vector<uint8_t> im_diagonals;
	bool enable_cache=1;

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

	void permute_diag(unsigned *iso)
	{
		const unsigned n = order;
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
			iso[8*n+i] = iso[i+n];
			iso[9*n+i] = iso[N-i];
					//lsq[5][o] = sq(N-j, i); 5
			iso[10*n+i] = iso[N-i+n];
			iso[11*n+i] = iso[i];
					//lsq[6][o] = sq(N-j, N-i); 6
			iso[12*n+i] = iso[N-i];
			iso[13*n+i] = iso[i+n];
					//lsq[7][o] = sq(i, N-j); 7
			iso[14*n+i] = iso[i+n];
			iso[15*n+i] = iso[i];
		}
	}

	void im_find_can(std::set<std::pair<unsigned,unsigned char>>* outset, unsigned* rule, const Square& sq)
	{
		/* Find the _rule_ of a input square and and all transformations that
		 * result in that rule. */
		unsigned tdiag[order*2*8];
		unsigned nmap[order];
		for(unsigned i=0; i<order; ++i) rule[i]=order;
		//for every diagonal transformation
		for(unsigned m=0; m < im_isotopes.size(); ++m) {
			//apply the transformation to our double-diagonal
			for(unsigned i=0; i<(order*2); ++i) {
				unsigned d = im_diagonals[m*order*2+i];
				tdiag[i] = sq [ d ];
			}
			//apply the 7 remaining simple permutations
			permute_diag(tdiag);
			for(unsigned t=0; t<8; ++t) {
				//this is the permuted diagonal
				unsigned* diag = &tdiag[order*2*t];
				//build normalization map
				for(unsigned i=0; i<order; ++i)
					nmap[diag[i]] = i;
				if(nmap[diag[order]]==0) {
					std::cerr<<"im_find_can: invalid anti-diagonal, m="<<m<<" t="<<t<<endl<<"t=0:";
					for(unsigned i=0; i<(order*2); ++i) std::cerr<<" "<<tdiag[i];
					std::cerr<<endl<<"t="<<t<<":";
					for(unsigned i=0; i<(order*2); ++i) std::cerr<<" "<<diag[i];
					std::cerr<<endl<<"mt:";
					for(unsigned i=0; i<(order*2); ++i) std::cerr<<" "<<unsigned(im_diagonals[m*order*2+i]);
					std::cerr<<endl;
					throw ESquareOp();
				}
				//normalize
				for(unsigned i=0; i<(order*2); ++i)
					diag[i] = nmap[ diag[i] ];
				//somehow the normalization was not applied here ???
				if(std::lexicographical_compare(diag+order,diag+(2*order),rule,rule+order)) {
					std::copy(diag+order, diag+order+order, rule);
					if(outset) {
						outset->clear();
						outset->emplace(m,t);
					}
				}
				else if(outset && std::equal(rule,rule+order,diag+order)) {
					outset->emplace(m,t);
				}
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

	Square Kanon(Square input_square)
	{
		const unsigned order = input_square.width();
		const unsigned N = input_square.width()-1;
		if(order!=this->order) {
			if(this->enable_cache && order>=12)
				use_cache(order);
				else init_order(order);
		}
		std::set<std::pair<unsigned,unsigned char>> mt_trans_set;
		unsigned mt_rule[order];
		std::set<Square> outset;
		im_find_can(&mt_trans_set, mt_rule, input_square);
		// rule was found, and also set of candidate transformations
		// try each transformation, this time on full square to find smallest square
		for( const auto& mt : mt_trans_set) {
			Square sq(input_square);
			for(unsigned i=0; i<=im_mtrans.size(); ++i) {
				if( (1LLU<<i) & im_isotopes[mt.first] ) {
					ApplyM(sq, im_mtrans[i]);
			}}
			sq=transform_sq(sq, mt.second).DiagNorm();
			if(0){
				std::cout<<"#K "<<im_isotopes[mt.first]<<" "<<(int)mt.second<<endl;
				unsigned cur_rule[order];
				for(unsigned i=0; i<order; ++i)
					cur_rule[i] = sq(i,N-i);
				assert(std::equal(mt_rule,mt_rule+order, cur_rule));
			}
			outset.insert(sq);
		}
		return *outset.begin();
	}

	void test_permutations()
	{
		const unsigned N = order-1;
		Square natural(order);
		for(unsigned i=0; i<natural.size(); ++i)
			natural[i] = i;
		unsigned diag[order*2*8];
		for(unsigned i=0; i<order; ++i)
			diag[i] = natural(i,i);
		for(unsigned i=0; i<order; ++i)
			diag[order+i] = natural(i,N-i);
		permute_diag(diag);
		for(unsigned t=0; t<8; ++t) {

			Square sq1 = transform_sq(natural, t);
			unsigned diag_1[order*2];
			for(unsigned i=0; i<order; ++i)
				diag_1[i] = sq1(i,i);
			for(unsigned i=0; i<order; ++i)
				diag_1[order+i] = sq1(i,N-i);

			if(!std::equal(diag_1,diag_1+order, diag+order*2*t)) {
				std::cerr<<"transform_diag broken on "<<t<<endl;
			}
		}
	}

	void init_order(unsigned order)
	{
		clock_t t0 = clock();
		const unsigned N = order-1;
		this->order= order;
		im_mtrans.clear();
		for(unsigned i=0; i< order/2; ++i) {
			im_mtrans.push_back(std::pair<int,int>{-1,i});
		}
		for(unsigned i=0; i< order/2; ++i)
			for(unsigned j=i+1; j< order/2; ++j) {
				im_mtrans.push_back(std::pair<int,int>{i,j});
		}
		test_permutations();
		Square natural(order);
		for(unsigned i=0; i<natural.size(); ++i)
			natural[i] = i;
		std::set<std::vector<uint8_t>> diag_izotopes;

		std::array<Square,8> lsq;
		for(int i=0; i<8; ++i)
			lsq[i]=Square(order);
		std::vector<bool> opts(im_mtrans.size(),0);
		std::vector<unsigned> stack(im_mtrans.size()+1);
		uint64_t transformation_index = 0;
		unsigned i = 0, p = 0;
		bool uniq;
		unsigned cur_iso[order*2*8];
		Square sq(natural);

		std::cerr<<"# KanonizerV("<<order<<"): transformations: "<<im_mtrans.size()<<" + 8, initializing..."<<endl;

		goto start;
		again:
			for(; i<im_mtrans.size() && opts[i]; ++i) {}
			if(i >= im_mtrans.size()) {
				if(!p) goto stop;
				i=stack[--p];
				opts[i]=0;
				ApplyM(sq, im_mtrans[i]);
				transformation_index ^= 1LLU<<i;
				i++;
				goto again;
			}
			ApplyM(sq, im_mtrans[i]);
			stack[p++] = i;
			opts[i]=1;
			transformation_index ^= 1LLU<<i;
		start:
			//std::cout<<"#m "<<transformation_index<<" p"<<p<<endl;

			for(unsigned i=0; i<order; ++i)
				cur_iso[i] = sq(i,i);
			for(unsigned i=0; i<order; ++i)
				cur_iso[order+i] = sq(i,N-i);
			permute_diag(cur_iso);

			uniq=true;
			for(unsigned k=0; k<8 && uniq; ++k) {
				std::vector<uint8_t> diag_izotope(cur_iso+(k*order*2),cur_iso+((k+1)*order*2));
				auto it = diag_izotopes.insert(diag_izotope);
				uniq= uniq && it.second;
				assert(!k || uniq);
			}
			if(uniq) {
				//std::cout<<"#M "<<transformation_index<<endl;
				im_isotopes.push_back( transformation_index );
				im_diagonals.insert( im_diagonals.end(), cur_iso, cur_iso+(order*2) );
			}
		goto again;
		stop:
		std::cerr<<"# KanonizerV("<<order<<"): m-isotopes: "<<im_isotopes.size()<<" *8 ("<<
		(double(clock() - t0) / CLOCKS_PER_SEC)<<"s)"<<endl;

	}

	void use_cache(int order)
	{
		std::ostringstream filename;
		filename <<"kanonb_cache_" <<order<< ".dat";
		std::ifstream cachein(filename.str(), ios::binary);
		if(cachein) {
			size_t sz_order, sz_mtrans, sz_diag, sz_isot;
			cachein >> sz_order >> sz_mtrans >> sz_isot;
			this->order=order;
			im_mtrans.resize(sz_mtrans);
			im_diagonals.resize(sz_diag = sz_isot*order*2 );
			cachein.ignore(4096, '\n');
			cachein.read(reinterpret_cast<char*>(im_mtrans.data()),sz_mtrans*sizeof(std::pair<int,int>));
			if(order>=16) {
				im_isotopes.resize(sz_isot);
				cachein.read(reinterpret_cast<char*>(im_isotopes.data()),sz_isot*sizeof(uint64_t));
			} else {
				std::vector<uint32_t> conv (sz_isot);
				cachein.read(reinterpret_cast<char*>(conv.data()),sz_isot*sizeof(uint32_t));
				im_isotopes = std::move(std::vector<uint64_t> (conv.begin(), conv.end()));
			}
			cachein.read(reinterpret_cast<char*>(im_diagonals.data()),sz_diag*sizeof(uint8_t));
			std::cerr<<"# KanonizerV: read "<<filename.str()<<": "<<sz_order<<" "<<sz_mtrans<<" "<<sz_isot<<" "<<sz_diag<<"\n";
		} else {
			init_order(order);
			std::cerr<<"# KanonizerV: write "<<filename.str()<<"\n";
			std::ofstream cacheo(filename.str(), ios::binary);
			cacheo << order <<" "<< im_mtrans.size() <<" "<< im_isotopes.size() <<"\n";
			cacheo.write(reinterpret_cast<char*>(im_mtrans.data()),im_mtrans.size()*sizeof(std::pair<int,int>));
			if(order>=16) {
				cacheo.write(reinterpret_cast<char*>(im_isotopes.data()),im_isotopes.size()*sizeof(uint64_t));
			} else {
				std::vector<uint32_t> conv (im_isotopes.begin(),im_isotopes.end());
				cacheo.write(reinterpret_cast<char*>(conv.data()),im_isotopes.size()*sizeof(uint32_t));
			}
			cacheo.write(reinterpret_cast<char*>(im_diagonals.data()),im_diagonals.size()*sizeof(uint8_t));
		}
	}

// order 11 - (2^15),   1920 uniq,	262144 (2^15*8) total transformations, 15360 unique ones (17x)
// order 12 - (2^21),  23040 uniq
// order 13 - (2^21),  23040 uniq
// order 14 - (2^28), 322560 uniq
// order 15 - (2^28), 
// order 16 - (2^36), 
// only the x is important
// store both diagonals and transorm(18 bits)
// find the minimal rule, foreach:
// buld normalization map, normalize antidiag
// opt works 11, 12  ... vymenit, otocit jednu, druhu

// in kanon_b find which transform leads to the desired result
// then in _v find why it was not applied

// create unit (A/B) tests for each function
// i suspect it might be in the diagonal permutation
};
