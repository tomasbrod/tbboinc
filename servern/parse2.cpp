
#include "parse2.hpp"
#include <stdio.h>
#include <string>
#include <cstring>
using std::string;


XML_PARSER2::XML_PARSER2(IStream* mf) {
	this->mf= mf;
	tag[0]=0;
	in_tag=0;
	is_closed=0;
	copy_end=copy_buf=0;
}

long XML_PARSER2::lookup(const char* table[], const size_t length, const char* needle)
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

static const char* Table[] = {
	"authenticator",
	"empty",
	"tag_name",
	"tree",
};

void XML_PARSER2::close_tag()
{
	while(in_tag)
	{
		int c= mf->r1();
		if(copy_buf<copy_end) *(copy_buf++)=c;
		if(in_tag>1) {
			if(in_tag==c) in_tag=1;
		}
		else if(c=='>') { in_tag=0; break; }
		else if(c=='"' || c=='\'') in_tag=c;
		is_closed=(c=='/');
	}
}

int XML_PARSER2::skip_ws(int c)
{
	while( isascii(c) && isspace(c) ) {
		c= mf->r1();
		if(copy_buf<copy_end) *(copy_buf++)=c;
	}
	return c;
}

int XML_PARSER2::scan_for(char* cur, size_t len, const char* delim, size_t* rsize)
{
	char* base = cur;
	char* end = cur+len;
	int c;
	try {
		while(11) {
			c= mf->r1();
			if(copy_buf<copy_end) *(copy_buf++)=c;
			for(unsigned i=0; delim[i]; ++i) if(delim[i]==c) goto stop;
			if(cur<end) (*cur++)=c;
		}
		stop:;
	}	catch(EStreamOutOfBounds& e) {c=EOF;}
	if(cur<end) *cur=0;
	if(len) end[-1]=0;
	if(rsize) *rsize= cur-base;
	//maybe we should return len if the output string was truncated
	return c;
}

static const char* entity_table[] = {
	"#xA", "#xD", "#xa", "#xd", 
	"amp", "apos", "gt",
	"lt", "quot",
};
static const char entity_values[] = {
	'\n', '\r', '\n', '\r',
	'&', '\'', '>',
	'<', '"',
};

int XML_PARSER2::unescape_for(char* cur, size_t len, char delim, int c, size_t* rsize)
{
	// copy one argument char
	// track entities
	// when etity, unescape using table and unhex
	char* base = cur;
	char* end = cur+len;
	char* amp = 0;
	try {
		if(!c) goto advance;
		again:
		if(cur<end) {
			if(c=='&') amp=cur;
			if(c==';' && amp) {
				*cur=0;
				long li= lookup(entity_table, sizeof entity_table, amp);
				if(li!=-1) {
					(*amp++)= entity_values[li];
					(*amp++)= 0;
					cur=amp;
				} else if((cur-amp)>2 && amp[1]=='#') {
					(*amp++)= '_';
					(*amp++)= 0;
				}
				else (*cur++)=c;
				amp=0;
			}
			else (*cur++)=c;
		}
		advance:
		c= mf->r1();
		if(copy_buf<copy_end) *(copy_buf++)=c;
		if(c!=delim) goto again;
	}	catch(EStreamOutOfBounds& e) {c=EOF;}
	if(cur<end) *cur=0;
	if(len) end[-1]=0;
	if(rsize) *rsize= cur-base;
	//maybe we should return len if the output string was truncated
	return c;
}

bool XML_PARSER2::get_tag(int c)
{
	try {
		do {
			// if in a tag, then close it (skip attributes)
			close_tag();
			// if this was self-closing tag - return false
			if(is_closed) {
				is_closed=0;
				return false;	}
			tag[0]=attr[0]=0;
			// if there is other text, skip it
			if(c!='<') {
				c= scan_for(0,0,"<");
			}
			//save pointer just before the tag, in case this is the closing tag of the tree
			copy_ptr=copy_buf-1;
			// scan tag
			size_t len;
			c= scan_for(tag, sizeof tag, "!> \n\t",&len);
			in_tag=(c!='>');
			if(c=='>') {
				if(len>1 && tag[0]!='/' && tag[len-1]=='/')
					is_closed=1;
			}
		} while(c=='!' && skip_comments(c));
		return (tag[0] && tag[0]!='/');
	}
	catch(EStreamOutOfBounds&) {return false;}
}


void XML_PARSER2::scan_attr(char* text, size_t len)
{
	//printf("  scan_attr: in_tag=%d [",in_tag);
	char* max = text + len - 1;
	if(len) text[0]=0;
	else   text=max=0;
	if(!in_tag) return;
	is_closed=0;
	// int c= skip_ws(' ');
	int c = skip_ws_close(' ');
	if(!in_tag)	return;
	//
	if(c=='"' || c=='\'') {
		in_tag=c;
		c= unescape_for(text,len,in_tag,0,0);
		in_tag=1;
	} else {
		if(len>1) {
			text[0]=c;
			text++; len--;
		}
		c= scan_for(text,len,"> \n\t",0);
		in_tag= (c!='>');
	}
	//printf("] in_tag=%d\n",in_tag);
}

int XML_PARSER2::skip_ws_close(int c)
{
	while( 1 ) {
		if(c=='>') {
			in_tag=0;
			return c;
		}
		else if( c=='/' ) is_closed=1;
		else if (!isascii(c) || !isspace(c)) break;
		c= mf->r1();
		if(copy_buf<copy_end) *(copy_buf++)=c;
	}
	return c;
}

bool XML_PARSER2::get_attr()
{
	try {
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
	catch(EStreamOutOfBounds&) {return false;}
}

bool XML_PARSER2::skip_comments(char c)
{
	//               0  3 5 7     13 16
	const char tr[]="!----> [CDATA[]]> > ";
	int i=0;
	while(1) {

		if(c==tr[i]) i++;
		else if(i==0) return false;
		else if(i==1 && c==tr[7]) i=8;
		else if(i<3) i=18;
		else if(i==5 && c==tr[4]) i=5;
		else if(i<=5) i=3;
		else if(i<=13) i=18;
		else if(i==16 && c==tr[15]) i=16;
		else if(i<=16) i=14;
		else if(i<=18) i=18;
		//fprintf(stderr,"skip_comments '%c' -> %d\n",c,i);

		if(tr[i]==' ') break;
		if(i==18 && c==tr[i]) break;

		c=mf->r1();
		if(copy_buf<copy_end) *(copy_buf++)=c;
	}
	is_closed=in_tag=0;
	return true;
}

void XML_PARSER2::get_tree(char* buf, size_t len)
{
	if(len>1) {
		copy_ptr=copy_buf=buf;
		copy_end=buf+len-1;
	}
	else if(len) {
		copy_end=copy_buf=buf;
	}
	for( int level=1; level; ) {
		if(get_tag()) {
			level++;
		} else {
			level--;
		}
	}
	if(len) {
		*copy_ptr=0;
		copy_end=copy_buf=0;
	}
}

void XML_PARSER2::get_str(char* str, size_t max)
{
	if(max) str[0]=0;
	try {
		if(in_tag>1) {
			scan_attr(str,max);
		} else if(max<2) {
			skip();
		} else {
			close_tag();
			if(is_closed) { is_closed=str[0]=0; return; }
			int c = skip_ws();
			if(c!='<') {
				size_t len=0;
				c= unescape_for(str,max,'<',c,&len);
				for(len=len+1-1; len; --len) {
					if(isspace(str[len]))
						str[len]=0;
				}
			}
			// reference behaviour is to throw if there are any other tags in the string
			// in ideal case, consistently return the unescaped contents
			// if there are any other tags, skip until end tag
			while(get_tag(c)) {
				//fprintf(stderr,"got tag in parse_str %s\n",tag);
				skip();
				c=0;
			}
		}
	}
	catch(EStreamOutOfBounds&) {};
}

void parse_test_1(XML_PARSER2& xp)
{
	char authenticator[32], tag_body[32];
	char enclosing_tag[TAG_BUF_LEN];
	char tree[1024];
	int tag_attr=0;
	tag_body[0]=authenticator[0]=0;
	//xp.save_enclosing(enclosing_tag);
	while(xp.get_tag()) {
		long ix = xp.lookup(Table,sizeof Table, xp.tag);
		printf("tag: %s n:%ld\n",xp.tag,ix);
		switch(ix) {
		case 0:
			xp.get_str(authenticator, sizeof authenticator);
			break;
		case 2:
			while(xp.get_attr()) {
				xp.get_str(tag_body, sizeof tag_body);
				printf(" attr: %s in_tag=%d text: %s\n",xp.attr,xp.in_tag,tag_body);
				/*if(!strcmp(xp.attr,"attr"))
					tag_attr= xp.parse_int();*/
			}
			xp.get_str(tag_body, sizeof tag_body);
			break;
		case 3:
			xp.get_tree(tree,sizeof(tree));
		default:
			xp.skip();
		}
	}
	//xp.assert_closing_tag(enclosing_tag);
	printf("authenticator = %s|\n",authenticator);
	printf("tag_attr = %d\n",tag_attr);
	printf("tag_body = %s|\n",tag_body);
	printf("tree = %s|\n",tree);
}

void parse_test(FILE* f) {
	bool flag;
	CFileStream mf(f);
	XML_PARSER2 xp(&mf);
	char name[64];
	char foo[64];
	int val;
	double x;
	string s;

	strcpy(name, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	/*if (!xp.parse_start("blah")) {
		printf("missing start tag\n");
		return;
	}*/
	xp.get_tag();
	parse_test_1(xp);
}



void parse_test_3(FILE* f) {
	CFileStream mf(f);
	XML_PARSER2 xp(&mf);
	char name[64];

	strcpy(name, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
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
