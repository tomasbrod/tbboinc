#pragma once
#include <iostream>
#include <sstream>
#include <mutex>

namespace StreamInternals
{
	template < class Stream, typename Arg > static void put_to_stream( Stream& ss, char join, const Arg& arg)
	{
		ss << arg;
		(void)join;
	}

	template < class Stream, typename Arg1, typename Arg2, typename ... ArgX > static void put_to_stream( Stream& ss, char join, const Arg1& arg1, const Arg2& arg2, const ArgX&... args )
	{
		put_to_stream(ss, join, arg1);
		ss << join;
		put_to_stream(ss, join, arg2, args...);
	}
};

template < typename ... Args > static std::string tostring( const Args&... args )
{
	std::stringstream ss;
	StreamInternals::put_to_stream(ss, ' ', args...);
	return ss.str();
}

static std::string tostring( const std::string& arg )
{
	return arg;
}

class CLog
{
	std::string ident;
	template < typename ... Args > void msg2( const CLog& arg, const Args&... args )
	{
		(*output) << ' ';
		(*output) << arg.ident_cstr();
		msg2(args...);
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
		StreamInternals::put_to_stream(*output, ' ', args...);
		this->endl2();
	}
	
	template < typename ... Args > void operator () ( const Args&... args ) {msg(args...);}

	template < typename ... Args > void error( const std::exception& e, const Args&... args )
	{
		std::lock_guard<std::mutex> lock (cs);
		put_exception(e);
		StreamInternals::put_to_stream(*output, ' ', args...);
		this->endl2();
	}

	template < typename ... Args > void warn( const Args&... args )
	{
		std::lock_guard<std::mutex> lock (cs);
		put_prefix(1);
		StreamInternals::put_to_stream(*output, ' ', args...);
		this->endl2();
	}

	explicit CLog() {}

	explicit CLog(const char* str);
	template < typename ... Args > explicit CLog(const Args&... args) {
		std::stringstream ss;
		StreamInternals::put_to_stream(ss, '.', args...);
		ident=ss.str();
	}

	const char* ident_cstr() const { return ident.c_str(); }

	friend std::stringstream& operator<<(std::stringstream& ss, const CLog& other);

	static std::ostream* output;
	static std::mutex cs;
	static bool timestamps;
};

class ERecoverable : public std::exception
{
	std::string msg;
	void init(CLog& log, const std::string& imsg);
	public:
	const char * what () const noexcept override;
	ERecoverable(const ERecoverable&) = default;
	template < typename ... Args > ERecoverable(CLog& log, const Args&... args )
	{
		init(log, tostring(args...));
	}
	template < typename ... Args > ERecoverable(const Args&... args )
	{
		CLog log("ERecoverable");
		init(log, tostring(args...));
	}
};

// convert anything to string

// TODO:
// trace macros ( HERE, VALUE(var) ) go to the singleton (use stringstream to stringify)
