
#include <stdio.h>
#include <string>
#include <cstring>

using std::string;

#include "boinclib/miofile.h"

#define TAG_BUF_LEN         4096

struct XML_PARSER2
{
	char tag [TAG_BUF_LEN];
	char attr[TAG_BUF_LEN];
	bool get_tag();
	bool get_attr();

	MIOFILE* mf;
	XML_PARSER2(MIOFILE* mf) {
		this->mf= mf;
		tag[0]=0;
		in_tag=0;
		is_closed=0;
	}
	void save_enclosing(char* c) {
		strcpy(c, tag);
	}
	void parse_str(char* str, size_t max);
	long parse_int();
	void assert_closing_tag(char* tag);
	int scan_for(char* text, size_t len, const char* delim);
	void close_tag();
	int skip_ws(int c=' ');
	void skip();
	void scan_attr(char* text, size_t len);
	int skip_ws_close(int c=' ');
	short in_tag; /* 0- not tag, i - in tag , ' , " */
	bool is_closed;
};

#if 0
  <tag name="value" >  text </tag>
  ^---^ Open
       ^---^ Attr
            ^-----^ AttrVal
#endif

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

const char* Table[] = {
	"authenticator",
	"empty",
	"tag_name",
};

void XML_PARSER2::close_tag()
{
	while(in_tag)
	{
		int c= mf->_getc();
		if(in_tag>1) {
			if(in_tag==c) in_tag=1;
		}
		else if(c=='>') { in_tag=0; break; }
		else if(c=='"' || c=='\'') in_tag=c;
		else if(c==EOF) in_tag=0;
		is_closed=(c=='/');
	}
}

int XML_PARSER2::skip_ws(int c)
{
	while( c!=EOF && isascii(c) && isspace(c) )
		c= mf->_getc();	
	return c;
}

int XML_PARSER2::scan_for(char* text, size_t len, const char* delim)
{
	int c;
	while(1) {
		c= mf->_getc();
		for(unsigned i=0; delim[i]; ++i) if(delim[i]==c) goto stop;
		if(c==EOF) break;
		if(len>1) {
			*text = c;
			len--; text++;
		}
	}
	stop:
	if(len) *text=0;
	return c;
}

bool XML_PARSER2::get_tag()
{
	// if in a tag, then close it (skip attributes)
	close_tag();
	// if this was self-closing tag - return false
	if(is_closed) {
		is_closed=0;
		return false;	}
	// skip whitespace
	int c = skip_ws();
	tag[0]=attr[0]=0;
	// if there is other text, skip it
	if(c!='<') {
		c= scan_for(0,0,"<");
	}
	if(c==EOF) return false;
	// scan tag
	c= scan_for(tag, sizeof tag, "> \n\t");
	if(c==EOF) return false;
	in_tag=(c!='>');
	if(c=='>') {
		c = strlen(tag);
		if(c && tag[c-1]=='/')
			is_closed=1;
	}
	return (tag[0] && tag[0]!='/');
}


void XML_PARSER2::scan_attr(char* text, size_t len)
{
	//printf("  scan_attr: in_tag=%d [",in_tag);
	char* max = text + len - 1;
	if(len) text[0]=0;
	else   text=max=0;
	if(!in_tag) return;
	is_closed=0;
	int c= skip_ws(' ');
	while(1) {
		//putc(c,stdout);
		if(c==EOF) break;
		else if(in_tag==2) {
			if(c=='"' || c=='\'') { in_tag=c; goto nextchar; }
			else if(isascii(c) && isspace(c)) {in_tag=1; break; }
		}
		else if(in_tag==c) { in_tag=1; break; }
		if(text<max)
			*(text++)=c;
		nextchar:
		c= mf->_getc();
	}
	if(len) *text=0;
	//printf("] in_tag=%d\n",in_tag);
}

int XML_PARSER2::skip_ws_close(int c)
{
	while( 1 ) {
		if(c=='>' || c==EOF) {
			in_tag=0;
			return c;
		}
		else if( c=='/' ) is_closed=1;
		else if (!isascii(c) || !isspace(c)) break;
		c= mf->_getc();
	}
	return c;
}

bool XML_PARSER2::get_attr()
{
	int c=' ';
	// in attribute? skip it (using parse_str or something)
	if(in_tag>1)
		scan_attr(0,0);
	if(!in_tag)
		return false;
	// whitespace
	c = skip_ws_close(c);
	if(!in_tag)	return false;
	// start of attr name
	attr[0]=c;
	c= scan_for(attr+1, sizeof(attr)-1, "=/> \n\t");
	// whitespace
	c = skip_ws_close(c);
	//
	if(c!='=') return in_tag = 0;
	in_tag=2;
	return true;
}

void XML_PARSER2::skip()
{
	for( int level=1; level; ) {
		if(get_tag()) {
			level++;
		} else {
			level--;
		}
	}
}

void XML_PARSER2::parse_str(char* str, size_t max)
{
	if(max) str[0]=0;
	if(in_tag>1) {
		scan_attr(str,max);
	} else if(max<2) {
		skip();
	} else {
		close_tag();
		if(is_closed) { is_closed=str[0]=0; return; }
		int c = skip_ws();
		str[0]=c;
		c= scan_for(str+1,max-1,"<");
		//TODO: should check the closing tag
		in_tag=1;
		//rtrim(str);
		//for( 1; str>=base; isspace(*(str--)) );
	}
}

void parse_test_1(XML_PARSER2& xp)
{
	char authenticator[32], tag_body[32];
	char enclosing_tag[TAG_BUF_LEN];
	int tag_attr=0;
	tag_body[0]=authenticator[0]=0;
	xp.save_enclosing(enclosing_tag);
	while(xp.get_tag()) {
		long ix = get_index(Table,sizeof Table, xp.tag);
		printf("tag: %s n:%ld\n",xp.tag,ix);
		switch(ix) {
		case 0:
			xp.parse_str(authenticator, sizeof authenticator);
			break;
		case 2:
			while(xp.get_attr()) {
				xp.parse_str(tag_body, sizeof tag_body);
				printf(" attr: %s in_tag=%d text: %s\n",xp.attr,xp.in_tag,tag_body);
				/*if(!strcmp(xp.attr,"attr"))
					tag_attr= xp.parse_int();*/
			}
			xp.parse_str(tag_body, sizeof tag_body);
			break;
		default:
			xp.skip();
		}
	}
	//xp.assert_closing_tag(enclosing_tag);
	printf("authenticator = %s\n",authenticator);
	printf("tag_attr = %d\n",tag_attr);
	printf("tag_body = %s\n",tag_body);
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
	do {
		if(xp.get_tag()) {
			printf("L%d %s\n",level,xp.tag,xp.in_tag);
			//xp.skip();
			while(xp.get_attr()) {
				printf(" attr: %s\n",xp.attr);
			}
			level++;
		} else {
			level--;
			//printf("L%d /\n",level);
		}
	}
	while(level>0);
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
