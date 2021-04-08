#pragma once

#include <string>
#include <array>
#include <vector>
#include <cstring>
#include <bitset>
#include "Stream.hpp"

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

	void get_string(std::string& str, size_t max);
	long get_long();
	bool get_bool();
	double get_double();
	unsigned long get_ulong();
	unsigned long long get_uquad();
	short get_enum_value(const char* table[], const size_t length);
	void halt() {in_tag=3;}

	static long lookup(const char* table[], const size_t length, const char* needle);
	static const char* array_too_long;
	static const char* duplicate_field;
	static const char* unknown_field;

	protected:
	bool get_tag(int c);
	int scan_for(char* text, size_t len, const char* delim, size_t* rsize=0);
	int unescape_for(char* text, size_t len, char delim, int c, size_t* rsize);
	void close_tag();
	int skip_ws(int c=' ');
	void scan_attr(char* text, size_t len);
	int skip_ws_close(int c=' ');
	bool skip_comments(char c);
};

struct EXmlParse : std::exception
{
	char buf[128];
	EXmlParse( const XML_PARSER2& _xp, const char* _msg );
	EXmlParse( const XML_PARSER2& _xp, bool _attr, const char* _name );
	const char * what () const noexcept;
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
	void clear() {(*this)[0]=0;}
	//todo: comparison...
};
