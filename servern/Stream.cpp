#include "Stream.hpp"
#include <unistd.h>
#include <stdexcept>

const char * EStreamOutOfBounds::what () const noexcept {return "Stream access Out Ouf Bounds";}

//struct CUnchStream : StreamInternals::PartOne<StreamInternals::Unchecked> {};
//struct IStream : StreamInternals::PartOne<StreamInternals::Virtual> {};

void CBufStream::setpos(size_t pos) { throw EStreamOutOfBounds(); }
byte* CBufStream::outofbounds(size_t len, bool wr){throw EStreamOutOfBounds();}

CFileStream::CFileStream(FILE* ifile) {file=fileno(ifile); buffer.reset(); chunk_pos=0;}
void CFileStream::flush()
{
	throw std::runtime_error("CFileStream flush unimplemented");
}
void CFileStream::setpos(size_t pos) {
	//if(pos==pos()) break;
	if(pos!=lseek(file, pos, SEEK_SET))
		throw std::runtime_error("CFileStream seek failed");
	chunk_pos=pos;
	ssize_t rc= read(file, static_buffer, sizeof(static_buffer));
	if(rc<0)
		throw std::runtime_error("CFileStream read failed");
	if(rc==0)
		throw EStreamOutOfBounds();
	buffer.reset(static_buffer,rc);
}
byte* CFileStream::outofbounds(size_t len, bool wr)
{
	if(len>sizeof(static_buffer))
		throw std::runtime_error("CFileStream read too big");
	buffer.cur-=len;
	setpos(pos());
	buffer.cur+=len;
	return buffer.base;
}

static void t() {
	IStream* i;
	CBufStream bs;
	//i->outofbounds(1,2);
}

// See also: https://docs.microsoft.com/en-us/cpp/cpp/explicit-instantiation?view=msvc-160
class CStreamzzzz // TODO
	: CUnchStream
{

	public:

	void read(void *dest, size_t len) {
		byte* x=this->getdata(len,0);
		std::copy(x,x+len,static_cast<byte*>(dest));
	}

	void write(const void *src, size_t len) {
		byte* x=this->getdata(len,1);
		std::copy(static_cast<const byte*>(src),static_cast<const byte*>(src)+len,x);
	}

	std::string ReadShortString() {
		size_t len = r1();
		return std::string ((char*)this->getdata(len,0), len);
	}

	std::string ReadStringAll() {
		size_t len = length() - pos();
		return std::string ((char*)this->getdata(len,0), len);
	}

	void WriteShortString(std::string s) {
		w1(s.length());
		std::copy(s.begin(),s.end(),(char*)this->getdata(s.length(),1));
	}
	
	void WriteStringAll(std::string s) {
		std::copy(s.begin(),s.end(),(char*)this->getdata(s.length(),1));
	}

};
