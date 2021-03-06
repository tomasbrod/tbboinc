#pragma once
#include <string>
#include <cstring>
#include <array>
#include <new>
using std::string;
#include "boinclib/miofile.h"
#include "boinclib/parse.h"

struct XML_PARSER2
	: XML_PARSER
{
	void unknown_tag();
	void ignore_tag();
	unsigned lookup_tag(const char* table[], const size_t tablesize);
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
	//todo: comparison...
};

long binary_search(const char* table[], const size_t tablesize, const char* needle);
