#include <cassert>
struct KanonizerV {
	unsigned order=0;
	std::vector<std::pair<int,int>> im_mtrans;
	std::vector<uint32_t> im_isotopes;
	std::vector<uint8_t> im_diagonals;

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

	void transform_diag(unsigned *iso)
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

	void im_find_can(std::set<std::pair<unsigned,unsigned char>>* outset, unsigned* rule, const Square& sq)
	{
		unsigned tdiag[order*2*8];
		unsigned nmap[order];
		for(unsigned i=0; i<order; ++i) rule[i]=order;
		for(unsigned m=0; m < im_isotopes.size(); ++m) {
			for(unsigned i=0; i<(order*2); ++i) {
				unsigned d = im_diagonals[m*order*2+i];
				tdiag[i] = sq [ d ];
			}
			transform_diag(tdiag);
			for(unsigned t=0; t<8; ++t) {
				unsigned* diag = &tdiag[order*2*t];
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
			init_order(order);
		}
		std::set<std::pair<unsigned,unsigned char>> mt_trans_set;
		unsigned mt_rule[order];
		std::set<Square> outset;
		im_find_can(&mt_trans_set, mt_rule, input_square);
		for( const auto& mt : mt_trans_set) {
			Square sq(input_square);
			for(unsigned i=0; i<=im_mtrans.size(); ++i) {
				if( (1<<i) & im_isotopes[mt.first] ) {
					ApplyM(sq, im_mtrans[i]);
			}}
			sq=transform_sq(sq, mt.second).DiagNorm();
			outset.insert(sq);
		}
		return *outset.begin();
	}

	void init_order(unsigned order)
	{
		clock_t t0 = clock();
		const unsigned N = order-1;
		this->order= order;
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

		std::cerr<<"# KanonizerV("<<order<<"): transformations: "<<im_mtrans.size()<<" + 8, initializing..."<<endl;

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
			transform_diag(cur_iso);

			bool uniq=true;
			for(unsigned k=0; k<8 && uniq; ++k) {
				std::vector<uint8_t> diag_izotope(cur_iso+(k*order*2),cur_iso+((k+1)*order*2));
				auto it = diag_izotopes.insert(diag_izotope);
				uniq= uniq && it.second;
			}
			if(uniq) {
				im_isotopes.push_back( transformation_index );
				im_diagonals.insert( im_diagonals.end(), cur_iso, cur_iso+(order*2) );
			}
		}
		std::cerr<<"# KanonizerV("<<order<<"): m-isotopes: "<<im_isotopes.size()<<" *8 ("<<
		(double(clock() - t0) / CLOCKS_PER_SEC)<<"s)"<<endl;

	}

// order 11 - (2^15),   1920 uniq,	262144 (2^15*8) total transformations, 15360 unique ones (17x)
// order 12 - (2^21),  23040 uniq
// order 13 - (2^21),  23040 uniq
// order 14 - (2^28), 322560 uniq
// order 15 - (2^  *8), 
// only the x is important
// store both diagonals and transorm(18 bits)
// find the minimal rule, foreach:
// buld normalization map, normalize antidiag
// opt works 11, 12  ... vymenit, otocit jednu, druhu
};
