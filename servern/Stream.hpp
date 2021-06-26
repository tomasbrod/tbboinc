#pragma once
#include <unistd.h>
#include <stdexcept>
#include <system_error>
#include <memory>
#include <string>
#include <array>
#include <string>
#include <cstring>

typedef unsigned char byte;
using std::unique_ptr;
template<typename T> using uptr = std::unique_ptr<T>;
template<typename T> using plain_ptr = T*;
using std::move;
class IStream;

struct EStreamOutOfBounds
	: std::exception
{ const char * what () const noexcept; };

struct CBuffer
{
	byte *base;
	byte *wend;
	byte *cur;

	size_t pos() const {
		return cur - base;
	}
	size_t length() const {
		return wend - base;
	}
	void reset() {
		base=wend=cur=0;
	}
	void reset(void* ptr, size_t size) {
		wend=base=cur= (byte*) ptr;
		wend += size;
	}
	void copyto(IStream* dest, size_t len);
	CBuffer() = default;
	explicit CBuffer(void* ptr, size_t size)
		: base((byte*)ptr), wend((byte*)ptr+size), cur((byte*)ptr+size)
		{}
	size_t left() {
		return wend - cur;
	}

	CBuffer operator+(unsigned dis)
	{
		return CBuffer(base+dis, cur-base-dis);
	}

	static void bin2hex(byte* dest, const void* ptr, size_t size);
	static inline void bin2hex(byte* dest, const CBuffer& data) {bin2hex(dest,data.base,data.length());}
	static bool hex2bin(byte* dest, const void* vptr, size_t len);
	//static inline void hex2bin(byte* dest, const CBuffer& data) {hex2bin(dest,data.base,data.length());}
};

template <std::size_t SIZE>
struct immstring : std::array< char, SIZE >
{
	operator char*() { return this->data(); }
	operator const char*() const { return this->data(); }
	immstring& operator = (const char* s) {
		strncpy(this->data(), s, SIZE-1);
		this->back()=0;
		return *this;
	}
	immstring& operator = (std::string& s) { return (*this) = s.c_str(); }
	immstring(const char* s) { *this=s; }
	immstring(std::string& s) { *this=s; }
	immstring() {};
	void clear() {(*this)[0]=0;}
	bool empty() const { return (*this)[0]==0; }
	//bool operator == (
	//todo: comparison...
};

namespace StreamInternals
{

	struct Unchecked
		: CBuffer
	{
		inline byte* getdata(size_t len, bool wr)
		{
			byte* ret= cur;
			cur+=len;
			return ret;
		}
		void setpos(size_t pos) {
			cur= base + pos;
		}
		void skip(long displacement) {
			cur= cur + displacement;
		}
		void read(void* buf, size_t len) {memcpy(buf,cur,len); cur+=len;}
		void write(const void* buf, size_t len) {memcpy(cur,buf,len); cur+=len;}
	};

	class Virtual
	{
		protected:
		virtual byte* outofbounds(size_t len, bool wr);
		virtual void read2(void* buf, size_t len);
		virtual void write2(const void* buf, size_t len);
		CBuffer buffer;
		size_t chunk_pos;
		public:
		virtual void setpos(ssize_t pos);
		byte* getdata(size_t len, bool wr)
		{
			byte* ret= buffer.cur;
			buffer.cur+=len;
			if(buffer.cur>buffer.wend)
				return outofbounds(len,wr);
			else return ret;
		}
		size_t pos() const {
			return chunk_pos + buffer.pos();
		}
		void skip(long dis)
		{
			if((buffer.cur+dis) > buffer.wend || (buffer.cur+dis) < buffer.base)
				setpos(pos()+dis);
			else buffer.cur+=dis;
		}
		void read(void* buf, size_t len)
		{
			if((buffer.cur+len) > buffer.wend)
				read2(buf,len);
			else {
				memcpy(buf,buffer.cur,len);
				buffer.cur+=len;
			}
		}
		void write(const void* buf, size_t len)
		{
			if((buffer.cur+len) > buffer.wend)
				write2(buf,len);
			else {
				memcpy(buffer.cur,buf,len);
				buffer.cur+=len;
			}
		}
		void write(const CBuffer& in) { write(in.base,in.length()); }
	};

	template <class Base>
	struct PartOne : Base
	{

		unsigned r1() {
			return this->getdata(1,0)[0];
		}

		void w1(unsigned v) {
			this->getdata(1,1)[0]=v;
		}

		unsigned r2() {
			byte*p=this->getdata(2,0);
			return (p[0])
				| (p[1]<<8);
		}
		
		void w2(unsigned v) {
			byte*p=this->getdata(2,1);
			p[0] = v, p[1]=v>>8;
		}

		unsigned rb2() {
			byte*p=this->getdata(2,0);
			return (p[1])
				| (p[0]<<8);
		}
		
		void wb2(unsigned v) {
			byte*p=this->getdata(2,1);
			p[1] = v, p[0]=v>>8;
		}
		
		unsigned r4() {
			byte*p=this->getdata(4,0);
			return (p[0])
			 | (p[1]<<8)
			 | (p[2]<<16)
			 | (p[3]<<24);
		}

		void w4(unsigned v) {
			byte*p=this->getdata(4,1);
			p[0]=v,
			p[1]=v>>8,
			p[2]=v>>16,
			p[3]=v>>24;
		}
		
		unsigned rb4() {
			byte*p=this->getdata(4,0);
			return (p[3])
			 | (p[2]<<8)
			 | (p[1]<<16)
			 | (p[0]<<24);
		}

		void wb4(unsigned v) {
			byte*p=this->getdata(4,1);
			p[3]=v,
			p[2]=v>>8,
			p[1]=v>>16,
			p[0]=v>>24;
		}
		
		unsigned long long r6() {
			byte* p=this->getdata(6,0);
			return (unsigned long long)(p[0])
			 | ((unsigned long long)p[1]<<8)
			 | ((unsigned long long)p[2]<<16)
			 | ((unsigned long long)p[3]<<24)
			 | ((unsigned long long)p[4]<<32)
			 | ((unsigned long long)p[5]<<40);
		}	

		void w6(unsigned long long v) {
			byte* p=this->getdata(6,1);
			p[0]=v,
			p[1]=v>>8,
			p[2]=v>>16,
			p[3]=v>>24,
			p[4]=v>>32,
			p[5]=v>>40;
		}

		unsigned long long r8() {
			byte* p=this->getdata(8,0);
			return (unsigned long long)(p[0])
			 | ((unsigned long long)p[1]<<8)
			 | ((unsigned long long)p[2]<<16)
			 | ((unsigned long long)p[3]<<24)
			 | ((unsigned long long)p[4]<<32)
			 | ((unsigned long long)p[5]<<40)
			 | ((unsigned long long)p[6]<<48)
			 | ((unsigned long long)p[7]<<56);
		}	

		void w8(unsigned long long v) {
			byte* p=this->getdata(8,1);
			p[0]=v,
			p[1]=v>>8,
			p[2]=v>>16,
			p[3]=v>>24,
			p[4]=v>>32,
			p[5]=v>>40,
			p[6]=v>>48,
			p[7]=v>>56;
		}

		//TODO: specialize, or remove this alltogether
		int getc()
		{
			try {
				return this->getdata(1,0)[0];
			} catch(EStreamOutOfBounds&) {
				return EOF;
			}
		}

		template <typename T, size_t N>	void writea(const std::array<T,N>& arr) {
			this->write(arr.data(),N);
		}
		template <typename T, size_t N>	void reada(std::array<T,N>& arr) {
			this->read(arr.data(),N);
		}

		void wstrf(const char* s, size_t size)
		{
			this->write(s,size-1);
		}
		void rstrf(char* s, size_t size)
		{
			this->read(s,size-1);
			s[size-1]=0;
		}
		template <std::size_t SIZE> void wstrf(const immstring<SIZE>& s) { wstrf(s, SIZE); }
		template <std::size_t SIZE> void rstrf(immstring<SIZE>& s) { rstrf(s,SIZE); }

		void wstrs(const char* s, size_t size) {
			this->w1(size); //todo: check
			this->write(s,size);
		}
		void wstrs(const char* s) { wstrs(s,strlen(s)); }
		void wstrs(const std::string& s) { wstrs(s.c_str(),s.size()); }
		void rstrs(char* s, size_t max) {
			size_t rsize = this->r1();
			if(rsize>=max) rsize=max-1; //todo: check
			this->read(s,rsize);
			s[rsize]=0;
		}
		template <std::size_t SIZE> void rstrs(immstring<SIZE>& s) { rstrs(s,SIZE); }
		char * rstrsa() {
			size_t rsize= this->r1();
			char* s = (char*) malloc(rsize+1);
			this->read(s,rsize);
			s[rsize]=0;
			return s;
		}
		void rstrs(std::string& s) { char tmp[256]; rstrs(tmp,256); s=std::string(tmp); }

		// * long prefix

		void writehex(const void* ptr, size_t size) { CBuffer::bin2hex(this->getdata(size*2,1),ptr,size); }
		void writehex(const CBuffer& data) {CBuffer::bin2hex(this->getdata(data.length()*2,1),data.base,data.length());}

	};

};

struct CUnchStream : StreamInternals::PartOne<StreamInternals::Unchecked> {};
struct IStream : StreamInternals::PartOne<StreamInternals::Virtual>
{
	virtual size_t left();
	virtual size_t length();
	virtual void copyto(IStream* dest, size_t len);
};

template <size_t SIZE>
struct CStUnStream : CUnchStream
{
	byte databuffer[SIZE];
	CStUnStream()
	{
		cur=base=databuffer;
		wend=base+SIZE;
	}
};

struct CBufStream : IStream
{
	// moveable from CBuffer
	CBufStream(const CBuffer& ibuf) {buffer=ibuf;}
	CBufStream() = default;
	// convert back to CBuffer derived object
	operator const CBuffer&() const { return this->buffer; }
	operator CBuffer&() { return this->buffer; }
	// implements IStream
	virtual void setpos(size_t pos);
	virtual void copyto(IStream* dest, size_t len) final;
	virtual size_t left() final;
	virtual size_t length() final;
	void allocate(size_t size);
	protected:
};

struct CHandleStream : IStream
{
	int file;
	bool writable;
	byte static_buffer[512];
	explicit CHandleStream(int ifile, bool iwr=0);
	CHandleStream() {file=-1; writable=0; buffer.reset();}
	~CHandleStream() { if(writable) flush(); }
	// flush method
	void flush();
	// implements IStream
	void copyto(CBufStream* dest, size_t len);
	void copyto(CHandleStream* dest, size_t len);
	virtual void copyto(IStream* dest, size_t len) final;
	protected:
	virtual byte* outofbounds(size_t len, bool wr) final;
	virtual void read2(void* buf, size_t len) final;
	virtual void write2(const void* buf, size_t len) final;
	void write3(const byte* buf, size_t len);
	size_t read3(byte* buf, size_t len);
};

struct CFileStream : CHandleStream
{
	CFileStream() : CHandleStream() {}
	void openRead(const char* filename);
	void openCreate(const char* filename);
	void openWrite(const char* filename);
	void close();
	virtual size_t length() final;
	~CFileStream() {close();}
	static void setReadOnly(const char* name, bool ro);
	//virtual void setpos(ssize_t pos) override;
	protected:
	size_t stat_length;
	void do_stat();
};

struct EFileAccess : std::system_error
{
	EFileAccess( int ev ) : std::system_error( ev, std::system_category()) {}
};

struct EFileNotFound : std::system_error
{
	EFileNotFound( int ev ) : std::system_error( ev, std::system_category()) {}
};

struct EDiskFull : std::system_error
{
	EDiskFull( int ev ) : std::system_error( ev, std::system_category()) {}
};

#pragma pack(push, 1)
template <typename T> class LEonLEtype
{
	T value;
	public:
	LEonLEtype(const T& i) : value(i) {}
	LEonLEtype() = default;
	operator T() const { return value; }
};

typedef LEonLEtype<uint16_t> LEuint16;
typedef LEonLEtype<uint32_t> LEuint32;
typedef LEonLEtype<uint64_t> LEuint64;
#pragma pack(pop)
