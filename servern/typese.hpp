#pragma once
#include <vector>
#include "Stream.hpp"

#include "log.hpp"
template<class T_, class Name=immstring<16>>
struct NamedPtr {
	using T = T_;
	Name name;
	T* ptr;
	T& operator->() { return *ptr; }
	NamedPtr& operator = (const char* s) {
		clear();
		name=s;
		return *this;
	}
	void clear() { name.clear(); ptr=0; }
	NamedPtr() {clear();}
};

template<class T>
void bind( NamedPtr<T>& ptr, std::vector<T>& list, CLog& log)
{
	for( T& el : list ) if (el.name==ptr.name) {
		ptr.ptr=&el;
		return;
	}
	throwNamedPtrNotFound(log, ptr.name, T::type_text);
}

typedef std::chrono::duration<long,std::ratio<1,64>> Ticks;

Ticks now();
void throwNamedPtrNotFound(CLog& log, const char* name, const char* type_text);
class XML_PARSER2;
class CBuffer;
