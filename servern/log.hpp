#include <iostream>
#include <mutex>
#include <typeinfo>
#include <cxxabi.h>

class CLog
{
	std::string ident;
	template < typename Arg, typename ... Args > void msg2( const Arg& arg, const Args&... args )
	{
		(*output) << ' ';
		(*output) << arg;
		msg2(args...);
	}
	void msg2()
	{
	}
	void endl2( )
	{
		(*output) << '\n';
	}
	void put_prefix(short severity);

	public:

	template < typename ... Args > void msg( const Args&... args )
	{
		std::lock_guard<std::mutex> lock (cs);
		put_prefix(0);
		this->msg2(args...);
		this->endl2();
	}
	
	template < typename ... Args > void operator () ( const Args&... args ) {msg(args...);}

	template < typename ... Args > void error( const std::exception& e, const Args&... args )
	{
		std::lock_guard<std::mutex> lock (cs);
		put_prefix(2);
		int status;
		const char * resolved_type_name = abi::__cxa_demangle(typeid(e).name(), 0, 0, &status);
		const char * type_name = resolved_type_name;
		if(!type_name) type_name = typeid(e).name();
		if(!type_name) type_name = "???";
		this->msg2(type_name, e.what(), args...);
		free((void*)resolved_type_name);
		this->endl2();
	}

	template < typename ... Args > void warn( const Args&... args )
	{
		std::lock_guard<std::mutex> lock (cs);
		put_prefix(1);
		this->msg2(args...);
		this->endl2();
	}

	// prepares prefix from it's constructor
	explicit CLog(const char* str);
	explicit CLog();
	void init(const CLog& parent, const char* str);
	void init(const char* fmt, ...);
	const char* ident_cstr() { return ident.c_str(); }

	static std::ostream* output;
	static std::mutex cs;
	static bool timestamps;
};

// trace macros ( HERE, VALUE(var) ) go to the singleton (use stringstream to stringify)

// the singleton forwards to either stderr or file
// and holds flag wheter to prefix timestamps

// Format
// CGI.4 Warn: text...

