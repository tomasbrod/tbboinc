#pragma once

#include <string>
#include <array>
#include <vector>
#include <cstring>
#include <bitset>
#include "Stream.hpp"

class XML_TAG4
{
	public:
	//friend class XML_TAG4;
	short in_tag = 0; /* 0- not tag, 1 in tag, 2 1+space, 3 attr eq, 6 - in body */
	IStream* mf = 0;
	std::string tag;
	unsigned indent = 0;
	void open();
	void attr2(const char* iattr);
	inline void attr_s(const char* s = 0);
	inline void attr_e();
	public:
	XML_TAG4() = default;
	explicit XML_TAG4(IStream* imf, const std::string&& itag)
		: mf(imf), tag(itag) { open(); }
	explicit XML_TAG4(XML_TAG4& iparent, const std::string&& itag)
		: mf(iparent.mf), tag(itag), indent(iparent.indent+1)
		{ iparent.body(1); open(); }
	explicit XML_TAG4(XML_TAG4& iparent, const char* itag)
		: mf(iparent.mf), tag(itag), indent(iparent.indent+1)
		{ iparent.body(1); open(); }
	~XML_TAG4() { close(); }
	void close();
	void put_raw(const char* buf, size_t len);
	void put(const char* str);
	void put(long v);
	void put_bool(bool v);
	void put(double v);
	//void put(unsigned long v);
	void put(unsigned long long v);
	void put_bsize(unsigned long long v);
	void put_enum(const char* table[], const size_t length, short v);
	void put(const std::string& str) {put(str.c_str());}
	void put(short v) {put((long)v);}
	XML_TAG4& attr(const char* iattr) { attr2(iattr); return *this; }
	//XML_TAG4& ctag(const char* itag) { return XML_TAG4(*this,itag); }
	XML_TAG4& operator = (XML_TAG4& s) = delete;
	void open(const std::string&& itag);
	IStream* body(bool line=false);
};

void XML_TAG_test();
