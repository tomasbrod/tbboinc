#pragma once
#include <iostream>
#include <mutex>

class CLog
{
	std::string ident;
	template < typename Arg, typename ... Args > void msg2( const Arg& arg, const Args&... args )
	{
		(*output) << ' ';
		(*output) << arg;
		msg2(args...);
	}
	template < typename ... Args > void msg2( const CLog& arg, const Args&... args )
	{
		(*output) << ' ';
		(*output) << arg.ident_cstr();
		msg2(args...);
	}
	void msg2()
	{
	}
	void endl2( )
	{
		(*output) << '\n';
		(*output).flush();
	}
	void put_prefix(short severity);
	void put_exception(const std::exception& e);

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
		put_exception(e);
		this->msg2(args...);
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
	const char* ident_cstr() const { return ident.c_str(); }

	static std::ostream* output;
	static std::mutex cs;
	static bool timestamps;
};

// TODO:
// trace macros ( HERE, VALUE(var) ) go to the singleton (use stringstream to stringify)
