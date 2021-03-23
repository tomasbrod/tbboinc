#pragma once
#include <unistd.h>
#include <stdexcept>

typedef unsigned char byte;

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

		//TODO: specialize, or remove this alltogether
		int getc()
		{
			try {
				return this->getdata(1,0)[0];
			} catch(EStreamOutOfBounds&) {
				return EOF;
			}
		}

	};

};

struct CUnchStream : StreamInternals::PartOne<StreamInternals::Unchecked> {};
struct IStream : StreamInternals::PartOne<StreamInternals::Virtual> {};

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

struct CBufStream : IStream
{
	// moveable from CBuffer
	CBufStream(const CBuffer& ibuf) {buffer=ibuf;}
	CBufStream() = default;
	// convert back to CBuffer derived object
	void release(CBuffer& obuf) {obuf=buffer;}
	// implements IStream
	virtual void setpos(size_t pos);
	protected:
	virtual byte* outofbounds(size_t len, bool wr);
};

struct CFileStream : IStream
{
	int file;
	byte static_buffer[512];
	CFileStream(int ifile) {file=ifile; buffer.reset(); chunk_pos=0;}
	CFileStream(FILE* ifile);
	// flush method
	void flush();
	// implements IStream
	virtual void setpos(size_t pos);
	protected:
	virtual byte* outofbounds(size_t len, bool wr);
};
