struct Kanonizer {
	Square last_sq[8];
	std::vector<std::pair<int,int>> optX;
	unsigned order=0;
	
	bool Last(Square& min, Square& sq, std::set<Square> *SOS)
	{
		const unsigned n = sq.width();
		const unsigned N = sq.width()-1;
		bool dupe = false;
		Square *lsq = last_sq;
		unsigned i=0;
		for(unsigned j=0; j<n; ++j) {
				lsq[7][sq(i, N-j)] = j;
				lsq[1][sq(N-i, j)] = j;
				lsq[2][sq(N-i, N-j)] = j;
				lsq[3][sq(j,i)] = j;
				lsq[4][sq(j, N-i)] = j;
				lsq[5][sq(N-j, i)] = j;
				lsq[6][sq(N-j, N-i)] = j;
				lsq[0][sq(i,j)] = j;
		}
		for(unsigned i=1, o=n; i<n; ++i) {
			for(unsigned j=0; j<n; ++j, ++o) {
				lsq[7][o] = lsq[7][sq(i, N-j)];
				lsq[1][o] = lsq[1][sq(N-i, j)];
				lsq[2][o] = lsq[2][sq(N-i, N-j)];
				lsq[3][o] = lsq[3][sq(j,i)];
				lsq[4][o] = lsq[4][sq(j, N-i)];
				lsq[5][o] = lsq[5][sq(N-j, i)];
				lsq[6][o] = lsq[6][sq(N-j, N-i)];
				lsq[0][o] = lsq[0][sq(i,j)];
			}
		}
		for(unsigned k=0; k<8; ++k) {
			for(unsigned j=0; j<n; ++j)
				lsq[k][j]=j;
			//std::cout<<"T"<<endl<<lsq[k];
			if(SOS && !dupe && !k) {
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
			//std::cerr<<"optX.size(): "<<optX.size()<<endl;
		}
		Square min(sq);
		unsigned i = 0, p = 0;
		stack.resize(optX.size()+1);
		opts.resize(optX.size(),0);

		Last(min, sq, SOS);

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

	unsigned KanonCnt(Square& min, const Square& in) {
		std::set<Square> SOS;
		min=Kanon(in, &SOS);
		return SOS.size();
	}
};
