#include "parse4.hpp"

#define ENOUGH 64

bool quote_all = 0;

inline void XML_TAG4::body(bool line)
{
	if(in_tag==1 || in_tag==2) {
		mf->w1('>');
		in_tag=6;
	}
	if(in_tag<6)
		throw std::runtime_error("wrong state for XML::body");
	if(line) {
		mf->w1('\n');
		in_tag=7;
	}
}

void XML_TAG4::attr_s(const char* s)
{
	if(in_tag>=6) return;
	if(in_tag==1 || in_tag==2) {
		mf->w1('>');
		in_tag=6;
		return;
	}
	if(in_tag!=3)
		throw std::runtime_error("wrong state for XML::attr_s");
	bool need = quote_all;
	if(!s) return;
	for(const char* p=s; !need && *p; ++p)
	{
		need |= !isalnum(*p);
	}
	if(need)
	{
		mf->w1('\'');
		in_tag=4;
	}
}

void XML_TAG4::attr_e()
{
	if(in_tag==4) {
		mf->w1('\'');
		in_tag=1;
	}
	if(in_tag==3) {
		mf->w1(' ');
		in_tag=2;
	}
}

void XML_TAG4::open()
{
	/*
	if(indent)
		mf->w1('\n');
	*/
	for(unsigned i=0; i<indent; ++i)
		mf->w1(' ');
	mf->w1('<');
	mf->write(tag.c_str(), tag.size());
	in_tag=1;
}

void XML_TAG4::put_raw(const char* buf, size_t len)
{
	body();
	mf->write(buf,len);
}
void XML_TAG4::put(const char* str)
{
	attr_s(str);
	for(const char* p = str; *p; ++p)
	{
		if(*p=='\'' && (in_tag<6))
			mf->write("&apos;",6);
		else if(*p=='\n' /* && (in_tag<6)*/)
			mf->write("&#xa;",5);
		else if(*p=='\r')
			mf->write("&#xd;",5);
		else if(*p=='&')
			mf->write("&amp;",5);
		else if(*p=='>')
			mf->write("&gt;",5);
		else if(*p=='<')
			mf->write("&lt;",5);
		else
			mf->w1(*p);
	}
	attr_e();
}
void XML_TAG4::put(long v)
{
	attr_s();
	ssize_t len = snprintf((char*)mf->getdata(ENOUGH,1),ENOUGH, "%ld", v);
	mf->skip(len-ENOUGH);
	attr_e();
}
void XML_TAG4::put_bool(bool v)
{
	attr_s();
	mf->w1(v?'1':'0');
	attr_e();
}

void XML_TAG4::put(double v)
{
	attr_s();
	ssize_t len = snprintf((char*)mf->getdata(ENOUGH,1),ENOUGH, "%.15f", v);
	mf->skip(len-ENOUGH);
	attr_e();
}

void XML_TAG4::put(unsigned long long v)
{
	attr_s();
	ssize_t len = snprintf((char*)mf->getdata(ENOUGH,1),ENOUGH, "%llu", v);
	mf->skip(len-ENOUGH);
	attr_e();
}
	
void XML_TAG4::put_bsize(unsigned long long v) {put(v);}

void XML_TAG4::put_enum(const char* table[], const size_t length, short v)
{
	attr_s();
	mf->write(table[v], strlen(table[v]));
	attr_e();
}

void XML_TAG4::attr2(const char* iattr)
{
	if(in_tag==1) {
		mf->w1(' ');
		in_tag=2;
	}
	if(in_tag!=2)
		throw std::runtime_error("wrong state for XML::attr");
	mf->write(iattr, strlen(iattr));
	mf->w1('=');
	in_tag= 3;
	if(quote_all) {
		mf->w1('\'');
		in_tag= 4;
	}
}

void XML_TAG4::close()
{
	if(in_tag==1) {
		mf->w1(' ');
		in_tag=2;
	}
	if(in_tag==2) {
		mf->w1('/');
		in_tag=0;
	} else
	if(in_tag>=6) {
		if(in_tag==7) {
			mf->w1('\n');
			for(unsigned i=0; i<indent; ++i)
				mf->w1(' ');
		}
		mf->w1('<');mf->w1('/');
		mf->write(tag.c_str(), tag.size());
		in_tag=0;
	}
	else if(in_tag)
		throw std::runtime_error("wrong state for XML::close");
	mf->w1('>');
	//
}

void XML_TAG_test()
{
	CHandleStream hs (STDOUT_FILENO, true);
	XML_TAG4 doc ( &hs, "document" );
	doc.attr("attr1").put("text1");
	XML_TAG4 sub = XML_TAG4( doc, "tag" );
	sub.attr("attr2").put(42.667);
	XML_TAG4(sub,"inner").put("value");
	XML_TAG4(sub,"nobody").attr("attr3").put("tex t3");
	//sub = XML_TAG4( doc, "another" );
}
