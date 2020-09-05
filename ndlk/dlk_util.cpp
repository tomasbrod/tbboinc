/* arbitrary-size DLK */

/* Struct to store LK */
class Square
{
public:
	typedef unsigned short elem_t;
	Square(size_t iCols)
		:mCols(iCols)
		,mData(iCols*iCols)
		{}
	Square() :mCols(0) {}
	/* Access */
	elem_t& operator()(size_t i, size_t j)
		{return mData.at(i * mCols + j);}
	elem_t  operator()(size_t i, size_t j) const
		{return mData[i * mCols + j];}
	elem_t& operator[](size_t p)
		{return mData[p];}
	elem_t operator[](size_t p) const
		{return mData[p];}
	elem_t& anti(size_t i)
		{return mData[i * mCols + (mCols-i-1)];}
	/* Meta */
	size_t width() const { return mCols; }
	size_t size() const { return mCols*mCols; }
	/* conversion */
	friend std::ostream& operator<<(std::ostream& os, const Square& dt);
	void Read(std::istream& is);
	void ReadAlnum(std::istream& is);
	std::ostream& WriteAlnum(std::ostream& os) const;
	operator string() const;
	string Encode();
	void Decode(const std::string& enc_str);
	/* Normalize */
	Square& Translit(const std::map<elem_t,elem_t>& tr);
	Square& Normaliz();
	Square& DiagNorm();
	/* Checks */
	bool isLK();
	bool isDLK();
	/* Transforms */
	Square Transpose() const;
	Square SwapYV() const;
	/* Compare */
	friend bool operator== (const Square &c1, const Square &c2);
	friend bool operator!= (const Square &c1, const Square &c2);
	friend bool operator< (const Square &c1, const Square &c2);
private:
	size_t mCols;
	std::vector<elem_t> mData;
};

struct ESquareOp	: std::exception { const char * what () const noexcept {return "Invalid Square for Operation";} };
struct ESquareDecode	: std::exception { const char * what () const noexcept {return "Square Decode error";} };

/* Compare */
bool operator== (const Square &c1, const Square &c2)
{
    return (c1.mData== c2.mData);
}
bool operator!= (const Square &c1, const Square &c2){return !(c1== c2);}
bool operator<  (const Square &c1, const Square &c2)
{
    return (c1.mData< c2.mData);
}

/* Reading and Writing LK */
std::ostream& operator<<(std::ostream& os, const Square& square)
{
	for(size_t i=0; i<square.width(); ++i) {
		for(size_t j=0; j<square.width(); ++j) {
			os<<square(i,j);
			if((j+1)<square.width()) os<<" ";
		}
		os<<std::endl;
	}
	return os;
}

void Square::Read(std::istream& is)
{
	std::string line;
	elem_t num;
	mData.resize(0);
	while(is && mData.empty()) {
		getline(is, line);
		std::istringstream iss(line);
		while(iss>>num) {
			mData.push_back(num);
		}
	}
	mCols = mData.size();
	mData.resize(size());
	for(size_t i=mCols; i<size(); ++i) {
		is>>num;
		mData[i]=num;
	}
}

std::ostream& Square::WriteAlnum(std::ostream& os) const
{
	const Square& square = *this;
	for(size_t i=0; i<square.width(); ++i) {
		for(size_t j=0; j<square.width(); ++j) {
			if(square(i,j)<=9)
				os<<char('0'+square(i,j));
			else
				os<<char('A'+square(i,j)-10);
			if((j+1)<square.width()) os<<" ";
		}
		os<<std::endl;
	}
	return os;
}

void Square::ReadAlnum(std::istream& is)
{
	mData.resize(0);
	char c;
	unsigned n=250;
	while(mData.size()<(n*n) && is) {
		is.get(c);
		if(c>='0' && c<='9')
			mData.push_back(c-'0');
		else if(c>='A' && c<='Z')
			mData.push_back(c-'A'+10);
		else if(c>='a' && c<='z')
			mData.push_back(c-'a'+10);
		else if(c=='\n' && n==250 && mData.size()) {
			n= mData.size();
			mCols=n;
			mData.reserve(n*n);
		}
	}
}

/* Normalize */
Square& Square::Translit(const std::map<elem_t,elem_t>& tr)
{
	for(size_t i=0; i<size(); ++i) {
		const auto& r= tr.find(mData[i]);
		if(tr.end()==r) throw ESquareOp(); // number not on the first line
		mData[i] = r->second;
	}
	return *this;
}
Square& Square::Normaliz()
{
	// Normalize on first line
	std::map<elem_t,elem_t> m;
	for(size_t i=0; i<width(); ++i) {
		const auto r = m.emplace(mData[i],i);
		if(!r.second)  throw ESquareOp();  // duplicate number on first line
	}
	return Translit(m);
}
Square& Square::DiagNorm()
{
	// Normalize on main diagonal
	std::map<elem_t,elem_t> m;
	for(size_t i=0, v=0; i<size(); i=i+width()+1, ++v) {
		const auto r = m.emplace(mData[i],v);
		if(!r.second)  throw ESquareOp();  // duplicate number on diagonal
	}
	return Translit(m);
}

/* Checking for LK, SN, DLK */
bool Square::isLK()
{
	std::set<elem_t> s;
	for(size_t i=0; i<width(); ++i) {
		s.clear();
		for(size_t j=0; j<width(); ++j) {
			const auto r = s.emplace((*this)(i,j));
			if(!r.second)
				return false;
		}
	}
	for(size_t i=0; i<width(); ++i) {
		s.clear();
		for(size_t j=0; j<width(); ++j) {
			const auto r = s.emplace((*this)(j,i));
			if(!r.second)
				return false;
		}
	}
	return true;
}

bool Square::isDLK()
{
	if(!isLK()) return false;
	std::set<elem_t> s;
	for(size_t i=0; i<size(); i=i+width()+1) {
		const auto r = s.emplace(mData[i]);
		if(!r.second) return false;
	}
	s.clear();
	for(size_t i=width()-1; i<size()-width(); i=i+width()-1) {
		const auto r = s.emplace(mData[i]);
		if(!r.second) return false;
	}
	return true;
}

/* Encoding */
static void mulBc(unsigned c, unsigned m, std::vector<unsigned short>& v, unsigned short B)
{
	if(0==c && 1==m) return;
	c = v[0] * m + c;
	v[0] = c % B; c /= B;
	for(size_t i=1; 1; ++i) {
		if(i==v.size()) {
			if(!c) break;
			v.push_back(0);
		}
		c += v[i]*m;
		v[i] = c % B;
		c /= B;
	}
}

static unsigned divB( unsigned d, std::vector<unsigned short>& v, unsigned short B)
{
	if(d==1)
		return 0;
	unsigned a = 0;
	size_t i = v.size()-1;
	do {
		a = a*B + v[i];
		v[i] = a / d;
		a = a % d;
	} while(i--);
	return a;
}

static const unsigned char base58[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const unsigned char debase58[128] = {
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
	255,0,1,2,3,4,5,6,7,8,255,255,255,255,255,255,
	255,9,10,11,12,13,14,15,16,255,17,18,19,20,21,255,
	22,23,24,25,26,27,28,29,30,31,32,255,255,255,255,255,
	255,33,34,35,36,37,38,39,40,41,42,43,255,44,45,46,
	47,48,49,50,51,52,53,54,55,56,57,255,255,255,255,255
};

std::string Square::Encode()
{
	Square& sq = *this;
	const unsigned n = sq.width();
	if(n>=28)
		throw ESquareOp();
	if(!sq.isDLK())
		throw ESquareOp();
	sq.Normaliz();
	Square out(sq);
	Square av(n);
	std::vector<std::set<unsigned>> usc(n+2); // used on column and two diagonals
	for(unsigned c=0; c<n; ++c) {
		av(0,c) = 1; // there is only one choice for first row of normalized square
		out(0,c) = 0;
		usc[c].insert(sq(0,c)); // use numbers from first row
	}
	usc[n+0]=usc[0];
	usc[n+1]=usc[n-1];
	for(unsigned r=1; r<n; ++r) {
		for(unsigned c=0; c<n; ++c) {
			std::set<unsigned> usv(usc[c]);
			if(r==c) {
				usv.insert(usc[n].begin(),usc[n].end());
				usc[n].insert(sq(r,c));
			}
			if(r==(n-1-c)) {
				usv.insert(usc[n+1].begin(),usc[n+1].end());
				usc[n+1].insert(sq(r,c));
			}
			for(unsigned c2=0; c2<c; ++c2) {
				usv.insert(sq(r,c2));
			}
			for(const auto uv : usv) {
				if(sq(r,c) > uv) {
					out(r,c) -= 1;
				}
			}
			av(r,c) = n - usv.size();
			usc[c].insert(sq(r,c));
		}
	}
	//std::cout<<av<<endl;
	//std::cout<<out<<endl;

	std::vector<unsigned short> v;
	v.push_back(0);

	size_t i = av.size()-1;
	do {
		mulBc(out[i],av[i],v,58);
		i--;
	} while(i>=n);
	mulBc(n,58,v,58);

	std::string r(v.size(),0);
	for(i=0; i<v.size(); ++i) r[i] = base58[v[i]];
	return r;
}

void Square::Decode(const std::string& enc_str)
{
	if(enc_str.empty()) throw ESquareDecode();
	std::vector<unsigned short> enc(enc_str.size());
	for( size_t i=0; i<enc_str.size(); ++i ) {
		enc[i] = debase58[enc_str[i]&127];
	}
	unsigned n = divB(58, enc, 58);
	//std::cout<<"n: "<<n<<endl;
	Square& out = *this;
	this->mCols=n;
	this->mData.resize(n*n);

	std::vector<std::set<unsigned>> usc(n+2); // used on column and two diagonals
	for(unsigned c=0; c<n; ++c) {
		out(0,c) = c; // first row is normalized
		usc[c].insert(c); // use numbers from first row
	}
	usc[n+0]=usc[0];
	usc[n+1]=usc[n-1];
	for(unsigned r=1; r<n; ++r) {
		for(unsigned c=0; c<n; ++c) {
			std::set<unsigned> usv(usc[c]);
			if(r==c)
				usv.insert(usc[n].begin(),usc[n].end());
			if(r==(n-1-c))
				usv.insert(usc[n+1].begin(),usc[n+1].end());
			for(unsigned c2=0; c2<c; ++c2) {
				usv.insert(out(r,c2));
			}
			unsigned m = n-usv.size();
			if(m<1) throw ESquareDecode();
			unsigned val = divB( m, enc, 58);
			//std::cout<<"m: "<<m<<" v: "<<val<<" (";
			for(const auto uv : usv) {
				//std::cout<<uv<<",";
				if(val >= uv) {
					val += 1;
				}
			}
			usc[c].insert(val);
			if(r==c)
				usc[n].insert(val);
			if(r==(n-1-c))
				usc[n+1].insert(val);
			//std::cout<<") r: "<<val<<endl;
			out(r,c) = val;
		}
	}
	if(!out.isDLK())
		throw ESquareDecode();
}

/* Some transformations */
Square Square::Transpose() const
{
	Square r(width());
	for(size_t i=0; i<width(); ++i)
		for(size_t j=0; j<width(); ++j)
			r(j,i) = (*this)(i,j);
	return r;
}
Square Square::SwapYV() const
{
	// must be normalized!!
	Square r(width());
	for(size_t y=0; y<width(); ++y)
		for(size_t x=0; x<width(); ++x)
			r(x,(*this)(x,y)) = y;
	return r;
}

