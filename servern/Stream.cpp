#pragma once
#include <unistd.h>
#include <stdexcept>

typedef unsigned char byte;

struct EStreamOutOfBounds
	: std::exception
{ const char * what () const noexcept {return "Stream access Out Ouf Bounds";} };

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
	void reset(byte* ptr, size_t size) {
		wend=base=cur= ptr;
		wend += size;
	}
	void reset(void* ptr, size_t size) {
		reset((byte*)ptr,size);
	}
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
		void skip(unsigned displacement) {
			cur= cur + displacement;
		}
		size_t left() {
			return wend - cur;
		}
	};

	template <class Base>
	struct PartOne : Base
	{

		unsigned r1() {
			return this->getdata(1,0)[0];
		}

		void w1(unsigned v) {
			this->getdata(1,1)[0]= v;
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

		int getc()
		{
			try {
				return this->getdata(1,0)[0];
			} catch(EStreamOutOfBounds&) {
				return EOF;
			}
		}

	};

	class Virtual
	{
		protected:
		virtual byte* outofbounds(size_t len, bool wr) =0;
		CBuffer buffer;
		size_t chunk_pos;
		public:
		byte* getdata(size_t len, bool wr)
		{
			byte* ret= buffer.cur;
			buffer.cur+=len;
			if(buffer.cur>buffer.wend)
				return outofbounds(len,wr);
			else return ret;
		}
		size_t pos() const { return chunk_pos + buffer.pos(); }
		virtual void setpos(size_t pos) =0;
		virtual void skip(unsigned displacement)
		{
			byte* p = buffer.cur + displacement;
			if(p >= buffer.base && p < buffer.wend)
				buffer.cur=p;
			else
				setpos(pos()+displacement);
		}
		//virtual size_t left() =0;
		//virtual size_t length() =0;
		//virtual void read(void* buf, size_t len) =0;
		//virtual void write(void* buf, size_t len) =0;
		virtual ~Virtual() {}
	};

};

struct CUnchStream : StreamInternals::PartOne<StreamInternals::Unchecked> {};

template <size_t SIZE>
struct CStUnStream : CUnchStream
{
	byte databuffer[SIZE];
	CStUnStream()
	{
		base=databuffer;
		wend=base+SIZE;
	}
};

struct IStream : StreamInternals::PartOne<StreamInternals::Virtual> {};

struct CBufStream : IStream
{
	// moveable from CBuffer
	CBufStream(const CBuffer& ibuf) {buffer=ibuf;}
	CBufStream() = default;
	// convert back to CBuffer derived object
	void release(CBuffer& obuf) {obuf=buffer;}
	// implements IStream
	virtual void setpos(size_t pos) { throw EStreamOutOfBounds(); }
	protected:
	virtual byte* outofbounds(size_t len, bool wr){throw EStreamOutOfBounds();}
};

struct CFileStream : IStream
{
	int file;
	byte static_buffer[512];
	CFileStream(int ifile) {file=ifile; buffer.reset(); chunk_pos=0;}
	CFileStream(FILE* ifile) {file=fileno(ifile); buffer.reset(); chunk_pos=0;}
	// flush method
	void flush();
	// implements IStream
	virtual void setpos(size_t pos) {
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
	protected:
	virtual byte* outofbounds(size_t len, bool wr)
	{
		if(len>sizeof(static_buffer))
			throw std::runtime_error("CFileStream read too big");
		buffer.cur-=len;
		setpos(pos());
		buffer.cur+=len;
		return buffer.base;
	}
};

static void t() {
	IStream* i;
	CBufStream bs;
	//i->outofbounds(1,2);
}



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
