
#include "parse3.hpp"
#include <stdio.h>
#include <string>
#include <cstring>
#include <array>
#include <new>
using std::string;
#include "boinclib/miofile.h"
#include "boinclib/parse.h"

void XML_PARSER2::unknown_tag()
{
	fprintf(stderr,"unknown tag %s\n",parsed_tag);
	ignore_tag();
}
void XML_PARSER2::ignore_tag()
{
	int depth = 1;
	char buf[ELEMENT_BUF_LEN];
	while(depth>0) {
		switch (get_aux(buf, ELEMENT_BUF_LEN-1, 0, 0)) {
			case XML_PARSE_EOF:
				throw EParseXML();
			case XML_PARSE_OVERFLOW:
				throw EParseXML();
			case XML_PARSE_TAG:
				fprintf(stderr,"ignore_tag: %s\n",buf);
				if(buf[0]=='/')
				depth--;
				else {
					int len = strlen(buf);
					if(len<1) throw EParseXML();
					if(buf[len-1]!='/')
						depth++;
				}
				break;
			case XML_PARSE_DATA:
			case XML_PARSE_CDATA:
			default:
				break;
		}
	}
}

long binary_search(const char* table[], const size_t tablesize, const char* needle)
{
	long l=0;
	long r= (tablesize/sizeof(ptrdiff_t))-1;
	while(l<=r) {
		long m = l + (r-l)/2;
		int c = strcmp(table[m],needle);
		if(!c) return m;
		else if(c<0) l = m+1;
		else r = m-1;
	}
	return -1;
}


unsigned XML_PARSER2::lookup_tag(const char* table[], const size_t tablesize)
{
	return binary_search(table, tablesize, parsed_tag);
}

size_t XML_PARSER2::parse_str(char* str, size_t len)
{
	if(! parse_str_aux(parsed_tag,str,len) )
		throw EParseXML();
	return 0; // todo: strlen
}

void XML_PARSER2::parse_string(std::string& str)
{
	char *buf=(char *)malloc(262144);
	if(!buf) throw std::bad_alloc();
	bool flag = parse_str_aux(parsed_tag, buf, 262144);
	if (flag) {
			str = buf;
	}
	free(buf);
	if(!flag) throw EParseXML();
}

long XML_PARSER2::parse_long()
{
	char buf[256], *end;
	parse_str(buf, sizeof buf);
	errno = 0;
	long val = strtol(buf, &end, 0);
	if (errno) throw EParseXML();
	if (*end) throw EParseXML();
	return val;
}

bool XML_PARSER2::parse_bool()
{
	return ( parse_long() > 0 );
}

double XML_PARSER2::parse_double()
{
	char buf[256], *end;
	parse_str(buf, sizeof buf);
	errno = 0;
	double val = strtod(buf, &end);
	if (errno) throw EParseXML();
	if (*end) throw EParseXML();
	return val;
}
	
unsigned long XML_PARSER2::parse_ulong()
{
	char buf[256], *end;
	parse_str(buf, sizeof buf);
	errno = 0;
	unsigned long val = strtoul(buf, &end, 0);
	if (errno) throw EParseXML();
	if (*end) throw EParseXML();
	return val;
}

unsigned long long XML_PARSER2::parse_uquad()
{
	char buf[256], *end;
	parse_str(buf, sizeof buf);
	errno = 0;
	unsigned long long val = boinc_strtoull(buf, &end, 0);
	if (errno) throw EParseXML();
	if (*end) throw EParseXML();
	return val;
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
