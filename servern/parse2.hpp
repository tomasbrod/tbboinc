#pragma once

#include <string>
#include "Stream.cpp"

#define TAG_BUF_LEN         4096

class XML_PARSER2
{
	public: //protected:
	short in_tag; /* 0- not tag, i - in tag , ' , " */
	bool is_closed;
	char*  copy_buf;
	char*  copy_ptr;
	char*  copy_end;
	IStream* mf;

	public:
	char tag [TAG_BUF_LEN];
	char attr[TAG_BUF_LEN];

	public:
	XML_PARSER2(IStream* mf);
	bool get_tag() { return get_tag(0); }
	bool get_attr();
	void skip() {get_tree(0,0);}
	void get_tree(char* buf, size_t len);
	void get_str(char* str, size_t max);

	static long lookup(const char* table[], const size_t length, const char* needle);

	protected:
	bool get_tag(int c);
	int scan_for(char* text, size_t len, const char* delim, size_t* rsize=0);
	int unescape_for(char* text, size_t len, char delim, size_t* rsize=0);
	void close_tag();
	int skip_ws(int c=' ');
	void scan_attr(char* text, size_t len);
	int skip_ws_close(int c=' ');
};
