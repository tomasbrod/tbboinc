#include "Stream.hpp"
#include "log.hpp"

#include <unistd.h>
#include <stdexcept>
#include <sys/stat.h>

#include <iostream>
#include <mutex>
#include <typeinfo>
#include <cxxabi.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>

const char * EStreamOutOfBounds::what () const noexcept {return "Stream access Out Ouf Bounds";}

//struct CUnchStream : StreamInternals::PartOne<StreamInternals::Unchecked> {};
//struct IStream : StreamInternals::PartOne<StreamInternals::Virtual> {};

void CBufStream::setpos(size_t pos) { throw EStreamOutOfBounds(); }
byte* CBufStream::outofbounds(size_t len, bool wr){throw EStreamOutOfBounds();}

void CFileStream::openRead(const char* filename)
{
	close();
	file = ::open( filename, O_RDONLY | O_CLOEXEC );
	if(file<0) {
		throw std::system_error( errno, std::system_category());
	}
}

void CFileStream::close()
{
	if(file>=0) {
		if(writable)
			flush();
		::close(file);
		file=-1;
	}
	writable=0;
	chunk_pos=0;
	buffer.reset();
}

//CFileStream::~CFileStream() {close();}

CHandleStream::CHandleStream(int ifile)
{
	file= ifile;
	buffer.reset();
	chunk_pos= 0;
	writable=0;
}

void CHandleStream::flush()
{
	throw std::runtime_error("CHandleStream flush unimplemented");
}
void CHandleStream::setpos(size_t pos) {
	//if(pos==pos()) break;
	if(pos!=lseek(file, pos, SEEK_SET))
		throw std::runtime_error("CHandleStream seek failed");
	chunk_pos=pos;
	ssize_t rc= read(file, static_buffer, sizeof(static_buffer));
	if(rc<0)
		throw std::runtime_error("CHandleStream read failed");
	if(rc==0)
		throw EStreamOutOfBounds();
	buffer.reset(static_buffer,rc);
}
byte* CHandleStream::outofbounds(size_t len, bool wr)
{
	if(len>sizeof(static_buffer))
		throw std::runtime_error("CHandleStream read too big");
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

std::ostream* CLog::output;
std::mutex CLog::cs;
bool CLog::timestamps;

void CLog::put_prefix(short severity)
{
	if(timestamps) {
		time_t time2 = time(0);
		struct tm time3;
		char buffer[64];
		strftime(buffer, sizeof buffer, "%m%d-%H:%M:%S ",
                       localtime_r(&time2, &time3));
		(*output) << buffer;
	}
	(*output) << ident;
	if(!severity)
		(*output) << ": ";
	else if(severity==1)
		(*output) << ": Warn: ";
	else if(severity==2)
		(*output) << ": Error: ";
}

CLog::CLog(const char* str) : ident(str) {}
#if 0
void CLog::init(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	char buffer[1024];
	vsnprintf(buffer, sizeof buffer, fmt, va);
	ident=buffer;
}
#endif

void CLog::put_exception(const std::exception& e)
{
		put_prefix(2);
		int status;
		const char * resolved_type_name = abi::__cxa_demangle(typeid(e).name(), 0, 0, &status);
		const char * type_name = resolved_type_name;
		if(!type_name) type_name = typeid(e).name();
		if(!type_name) type_name = "???";
		StreamInternals::put_to_stream(*output, ' ', type_name, e.what(), "");
		free((void*)resolved_type_name);
}

std::stringstream& operator<<(std::stringstream& ss, const CLog& other)
{
	ss << other.ident;
	return ss;
}

// trace macros ( HERE, VALUE(var) ) go to the singleton (use stringstream to stringify)

// the singleton forwards to either stderr or file
// and holds flag wheter to prefix timestamps

// Format
// CGI.4 Warn: text...

