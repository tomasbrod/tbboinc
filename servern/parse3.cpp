
#include <stdio.h>
#include <string>
#include <cstring>
#include <array>

using std::string;

#include "boinclib/miofile.h"
#include "boinclib/parse.h"

#define TAG_BUF_LEN         4096

struct XML_PARSER2
	: XML_PARSER
{
	void unknown_tag();
	void ignore_tag();
	size_t parse_str(char* str, size_t len);
	void parse_string(std::string& str);
	long parse_long();
	bool parse_bool();
	double parse_double();
	unsigned long parse_ulong();
	unsigned long long parse_uquad();
	XML_PARSER2(MIOFILE* mf) : XML_PARSER(mf) {}
};

struct EParseXML
	//: std::runtime_error
{
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
};

void f()
{
	immstring<10> str;
	immstring<10> str2;
	str2 = str = "test";
}

long get_index(const char* table[], const size_t length, const char* needle)
{
	long l=0;
	long r= (length/sizeof(ptrdiff_t))-1;
	while(l<=r) {
		long m = l + (r-l)/2;
		int c = strcmp(table[m],needle);
		if(!c) return m;
		else if(c<0) l = m+1;
		else r = m-1;
	}
	return -1;
}

#include "build/defs.hpp"
#include "build/defs.cpp"

void parse_test_1(XML_PARSER2& xp)
{

}

void parse_test(FILE* f) {
	bool flag;
	MIOFILE mf;
	XML_PARSER2 xp(&mf);
	char name[64];
	char foo[64];
	int val;
	double x;
	string s;

	strcpy(name, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	mf.init_file(f);
	/*if (!xp.parse_start("blah")) {
		printf("missing start tag\n");
		return;
	}*/
	xp.get_tag();
	parse_test_1(xp);
}



void parse_test_3(FILE* f) {
	MIOFILE mf;
	XML_PARSER2 xp(&mf);
	char name[64];

	strcpy(name, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	mf.init_file(f);
	int level = 0;
}


/* try it with something like:

<?xml version="1.0" encoding="ISO-8859-1" ?>
<blah>
	<x>
	asdlfkj
	  <x> fj</x>
	</x>
	<str>blah</str>
	<int>  6
	</int>
	<double>6.555</double>
	<bool>0</bool>
</blah>

*/
