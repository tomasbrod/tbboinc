#pragma once
#include <vector>
#include "Stream.hpp"

#include "log.hpp"
template<class T_, class Name=immstring<16>>
struct NamedPtr {
	using T = T_;
	Name name;
	T* ptr;
	T* operator->() const { return ptr; }
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
typedef std::chrono::duration<uint32_t,std::ratio<64,1>> Dings;

Ticks now();
Dings nowd();
void throwNamedPtrNotFound(CLog& log, const char* name, const char* type_text);
class XML_PARSER2;
class XML_TAG4;
class CBuffer;


template<class T_>
struct IdedPtr {
	using T = T_;
	short id;
	T* ptr;
	T* operator->() { return ptr; }
	IdedPtr& operator = (short s) {
		clear();
		id=s;
		return *this;
	}
	void clear() { id=0; ptr=0; }
	IdedPtr() {clear();}
};
