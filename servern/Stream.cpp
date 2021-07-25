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
#include <assert.h>


const char * EStreamOutOfBounds::what () const noexcept {return "Stream access Out Ouf Bounds";}

//struct CUnchStream : StreamInternals::PartOne<StreamInternals::Unchecked> {};
//struct IStream : StreamInternals::PartOne<StreamInternals::Virtual> {};

void StreamInternals::Virtual::read2(void* buf, size_t len) { throw EStreamOutOfBounds(); }
void StreamInternals::Virtual::write2(const void* buf, size_t len) { throw EStreamOutOfBounds(); }
void StreamInternals::Virtual::setpos(ssize_t pos2) { throw EStreamOutOfBounds(); }
byte* StreamInternals::Virtual::outofbounds(size_t len, bool wr) {throw EStreamOutOfBounds();}
/*{
	byte* p = buffer.cur + displacement;
	if(p >= buffer.base && p < buffer.wend)
		buffer.cur=p;
	else
		setpos(pos()+displacement);
}*/
size_t IStream::left() { return SIZE_MAX; }
size_t IStream::length() { return 0; }

void IStream::copyto(IStream* dest, size_t len)
{
	while(size_t cnt = std::min(len, (size_t)(buffer.wend - buffer.cur) )) {
		dest->write(buffer.cur,cnt);
		buffer.cur = buffer.wend;
		len -= cnt;
		setpos(pos()); //FIXME: formal interface for advancing buffer
	}
}

void CBuffer::copyto(IStream* dest, size_t len)
{
	if(len > (wend-cur))
		len=(wend-cur);
	dest->write(cur,len);
	cur=wend;
}

void CBufStream::setpos(size_t pos) { throw EStreamOutOfBounds(); }
size_t CBufStream::left() { return buffer.wend - buffer.cur; }
size_t CBufStream::length() { return buffer.length(); }

void CBufStream::copyto(IStream* dest, size_t len)
{
	len = std::min(left(),len);
	// getdata is instant for CBufStream, use it
	dest->write(this->getdata(len,0),len);
}

void CBufStream::allocate(size_t size)
{
	assert(false);
}


void CFileStream::openRead(const char* filename)
{
	close();
	file = ::open( filename, O_RDONLY | O_CLOEXEC );
	if(file<0) {
		if(errno==ENOENT)
			throw EFileNotFound(errno);
		else throw std::system_error( errno, std::system_category());
	}
	do_stat();
}

void CFileStream::openWrite(const char* filename)
{
	close();
	file = ::open( filename, O_RDWR | O_CLOEXEC | O_CREAT, 0664 );
	if(file<0) {
		if(errno==EACCES)
			throw EFileAccess(errno);
		else if(errno==ENOENT)
			throw EFileNotFound(errno);
		else throw std::system_error( errno, std::system_category());
	}
	writable=1;
	do_stat();
}

void CFileStream::openCreate(const char* filename)
{
	close();
	file = ::open( filename, O_RDWR | O_CLOEXEC | O_CREAT | O_TRUNC , 0664 );
	if(file<0) {
		if(errno==EACCES)
			throw EFileAccess(errno);
		else if(errno==ENOENT)
			throw EFileNotFound(errno);
		else throw std::system_error( errno, std::system_category());
	}
	writable=1;
	stat_length = 0;
}

void CFileStream::do_stat()
{
	stat_length = 0; //TODO
	struct ::stat statbuf;
	if(::fstat(file,&statbuf)<0)
		throw std::system_error( errno, std::system_category());
	stat_length = statbuf.st_size;
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
size_t CFileStream::length() { return stat_length; }

CHandleStream::CHandleStream(int ifile, bool iwr)
{
	file= ifile;
	buffer.reset();
	chunk_pos= 0;
	writable=iwr;
	//FIXME: the chunk_pos is wrong on readable handles
	/*if(!writeable) {
		outofbounds(0,0);
		chunk_pos=0;
	}*/
}

void CFileStream::setReadOnly(const char* name, bool ro)
{
	mode_t mode = 0664;
	if(ro) mode = 0444;
	if(::chmod(name, mode )<0)
		throw std::system_error( errno, std::system_category());
}

void CHandleStream::write3(const byte* buf, size_t len)
{
	while(len) {
		ssize_t rc = ::write(this->file, buf, len);
		if(rc<=0)
			throw std::system_error( errno, std::system_category());
		buf+=rc;
		len-=rc;
		chunk_pos += rc;
	}
}

size_t CHandleStream::read3(byte* buf, size_t ilen)
{
	size_t len=ilen;
	while(len) {
		ssize_t rc = ::read(this->file, buf, len);
		if(rc<0)
			throw std::system_error( errno, std::system_category());
		if(rc==0) break;
		buf+=rc;
		len-=rc;
		chunk_pos += rc;
	}
	return ilen - len;
}

void CHandleStream::read2(void* ibuf, size_t len)
{
	byte* buf = (byte*)ibuf;
	// read as much from buffer
	size_t buflen = std::min(buffer.left(),len);
	if(buflen) {
		memcpy(buf,buffer.cur,buflen);
		buffer.cur += buflen;
		buf += buflen;
		len -= buflen;
	}
	// read rest from file directly
	if(len) {
		read3(buf,len);
	}
}

void CHandleStream::flush()
{
	if(!writable) return;
	write3(buffer.base, buffer.pos());
	buffer.reset(static_buffer,sizeof(static_buffer));
}

void CHandleStream::write2(const void* ibuf, size_t len)
{
	if(!writable)
		throw std::runtime_error("CHandleStream is not writable");
	flush();
	write3((byte*)ibuf,len);
}

/*
void CHandleStream::setpos(size_t pos) {
	//if(pos==pos()) break;
	if(pos!=lseek(file, pos, SEEK_SET))
		throw std::runtime_error("CHandleStream seek failed");
	chunk_pos=pos;
	ssize_t rc= ::read(file, static_buffer, sizeof(static_buffer));
	if(rc<0)
		throw std::runtime_error("CHandleStream read failed");
	if(rc==0)
		throw EStreamOutOfBounds();
	buffer.reset(static_buffer,rc);
}
*/
byte* CHandleStream::outofbounds(size_t len, bool wr)
{
	if(len>sizeof(static_buffer))
		throw std::runtime_error("CHandleStream read too big");
	if(wr && !writable)
		throw std::runtime_error("CHandleStream is not writable");
	if(wr) {
		assert(buffer.pos()>=len);
		write3(buffer.base, buffer.pos()-len);
		buffer.reset(static_buffer,sizeof(static_buffer));
		buffer.cur=buffer.base+len;
		return buffer.base;
	} else {
		buffer.cur-=len;
		// copy leftover data
		assert(buffer.left()<sizeof(static_buffer));
		//FIXME: somehow this aborts during legitimate use
		size_t left;
		for(left=0; (buffer.cur+left)<buffer.wend; ++left)
			static_buffer[left] = buffer.cur[left];
		buffer.base=static_buffer;
		buffer.wend=buffer.base+left;
		buffer.cur= buffer.base+len;
		size_t rc = read3(buffer.wend, sizeof(static_buffer) - left);
		buffer.wend+=rc;
		if(buffer.cur>buffer.wend)
			throw EStreamOutOfBounds(); // EOF reached
		return buffer.base;
	}
}

void CHandleStream::copyto(CBufStream* dest, size_t len)
{
	// getdata is instant for CBufStream, use it
	this->read(dest->getdata( len, 1), len );
}
	
void CHandleStream::copyto(CHandleStream* dest, size_t len)
{
	size_t cnt;
	if( cnt = std::min(len, (size_t)(buffer.wend - buffer.cur) )) {
		dest->write(buffer.cur,cnt);
		buffer.cur = buffer.wend;
		len -= cnt;
	}
	if(len) {
		int pipefd[2];
		if(::pipe(pipefd)<0)
			throw std::system_error( errno, std::system_category());
		cnt = 0;
		do {
			ssize_t rc = ::splice( this->file, NULL, pipefd[0], NULL, len, 0);
			printf("splice %ld of %lu data in\n", rc, len);
			if(rc<0)
				throw std::system_error( errno, std::system_category());
			if(rc==0)
				break;
			len -= rc;
			cnt += rc;
			ssize_t wc = ::splice( pipefd[1], NULL, this->file, NULL, cnt, 0);
			printf("splice %ld of %lu data in\n", wc, cnt);
			if(wc<0)
				throw std::system_error( errno, std::system_category());
		} while(len);
		while(cnt) {
			ssize_t wc = ::splice( pipefd[1], NULL, this->file, NULL, cnt, 0);
			printf("splice %ld of %lu data in\n", wc, cnt);
			if(wc<0)
				throw std::system_error( errno, std::system_category());
			cnt -= wc;
		}
	}
}

void CHandleStream::copyto(IStream* dest, size_t len)
{
	len = std::min(left(),len);
	if(auto bufstream = dynamic_cast<CBufStream*>(dest))
		CHandleStream::copyto(bufstream,len);
	else if(auto handlestream = dynamic_cast<CHandleStream*>(dest))
		CHandleStream::copyto(handlestream,len);
	else
		IStream::copyto(dest,len);
}

void CBuffer::bin2hex(byte* dest, const void* vptr, size_t len)
{
	byte* ptr = (byte*)vptr;
	while(len)
	{
		snprintf((char*)dest,3,"%02hhx",*ptr);
		if((ptr[0]>>4)<10)
			dest[0]=(ptr[0]>>4)+'0';
		else
			dest[0]=(ptr[0]>>4)-10+'a';
		if((ptr[0]&15)<10)
			dest[1]=(ptr[0]&15)+'0';
		else
			dest[1]=(ptr[0]&15)-10+'a';
		dest+=2;
		ptr++;
		len--;
	}
}

bool CBuffer::hex2bin(byte* dest, const void* vptr, size_t len)
{
	byte* ptr = (byte*)vptr;
	while(len)
	{
		byte n = 0;
		if(ptr[0]>='0'&&ptr[0]<='9')
			n =  (ptr[0]-'0')<<4;
		else if(ptr[0]>='a'&&ptr[0]<='f')
			n =  (ptr[0]-'a'+10)<<4;
		else if(ptr[0]>='A'&&ptr[0]<='F')
			n =  (ptr[0]-'A'+10)<<4;
		else return false;
		if(ptr[1]>='0'&&ptr[1]<='9')
			n |= (ptr[1]-'0');
		else if(ptr[1]>='a'&&ptr[1]<='f')
			n |= (ptr[1]-'a'+10);
		else if(ptr[1]>='A'&&ptr[1]<='F')
			n |= (ptr[1]-'A'+10);
		else return false;
		*(dest++)=n;
		ptr+=2;
		len-=2;
	}
	return true;
}

static void t() {
	IStream* i;
	CBufStream bs;
	//i->outofbounds(1,2);
}

// See also: https://docs.microsoft.com/en-us/cpp/cpp/explicit-instantiation?view=msvc-160

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

void ERecoverable::init(CLog& log, const std::string& imsg)
{
	msg = tostring(imsg,"from",log);
	log.warn(imsg);
}

const char * ERecoverable::what () const noexcept
{
	return msg.c_str();
}

// trace macros ( HERE, VALUE(var) ) go to the singleton (use stringstream to stringify)

// the singleton forwards to either stderr or file
// and holds flag wheter to prefix timestamps

// Format
// CGI.4 Warn: text...

