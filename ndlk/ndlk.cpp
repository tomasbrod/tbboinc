#include <cstddef>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
using std::string;
using std::endl;
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
	elem_t& operator()(size_t p)
		{return mData.at(p);}
	/* Meta */
	size_t width() const { return mCols; }
	size_t size() const { return mCols*mCols; }
	/* conversion */
	friend std::ostream& operator<<(std::ostream& os, const Square& dt);
	void Read(std::istream& is);
	operator string() const;
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
private:
	size_t mCols;
	std::vector<elem_t> mData;
};

struct ESquareOp	: std::exception { const char * what () const noexcept {return "Invalid Square for Operation";} };

/* Reading and Writing LK */
std::ostream& operator<<(std::ostream& os, const Square& square)
{
	for(size_t i=0; i<square.width(); ++i) {
		for(size_t j=0; j<square.width(); ++j) {
			os<<square(i,j);
			if((j+1)<square.size()) os<<" ";
		}
		if((i+1)<square.size()) os<<std::endl;
	}
	return os;
}

void Square::Read(std::istream& is)
{
	std::string line;
	elem_t num;
	mData.resize(0);
	getline(is, line);
	std::istringstream iss(line);
	while(iss>>num) {
		mData.push_back(num);
	}
	mCols = mData.size();
	mData.resize(size());
	for(size_t i=mCols; i<size(); ++i) {
		is>>num;
		mData[i]=num;
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
			if(!r.second) return false;
		}
	}
	for(size_t i=0; i<width(); ++i) {
		s.clear();
		for(size_t j=0; j<width(); ++j) {
			const auto r = s.emplace((*this)(j,i));
			if(!r.second) return false;
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

/* enumerate transversals, dtrans, disjoint trans */
/* ortogon_u can build ODLK of 12, maybe others? */

#include "exact_cover_u.cpp"

int main(int argc, char* argv[])
{
	Square sq;
	sq.Read(std::cin);
	std::cout<<"width: "<<sq.width()<<endl;
	std::cout<<"isLK: "<<sq.isLK()<<endl;
	std::cout<<"isDLK: "<<sq.isDLK()<<endl;
	//sq= sq.Transpose().SwapYV();
	sq.Normaliz();
	std::cout<<sq;
	return 69;
}
