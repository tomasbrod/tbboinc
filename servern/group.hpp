#pragma once
#include "kv.hpp"
#include <string>
#include <assert.h>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <map>
#include "Stream.hpp"
#include "log.hpp"

using std::string;;


struct COutput;
class GroupCtl;

struct TTask
{
	unsigned id;
	IdedPtr<COutput> output;
	GroupCtl* group;
};


struct GroupCtl
{
	std::mutex cs;
	std::condition_variable cv;
	unsigned nopen;
	Ticks open_since;
	std::map<immstring<16>,unique_ptr<KVBackend>> dbs;
	struct IdMapElem {
		immstring<8> type_text;
		immstring<16> name;
		short id;
		short len;
	};
	std::vector<IdMapElem> IdMapVec;
	const Ticks group_delay = Ticks(std::chrono::seconds(3));
	KVBackend* main;
	CLog log;

	void init(CLog& ilog, std::vector<t_config_database1>& cfgs);

	KVBackend* getKV(const immstring<16>& name);

	short getID(const char* type_text, const char* name, byte len=1);

	void Open();

	void Close();

	void ActuallyCommit();

	void releaseTask(TTask *p);

	struct TaskPtr_deleter {
		void operator()(TTask *p);
	};

	class TaskPtr
		: public std::unique_ptr<TTask, GroupCtl::TaskPtr_deleter >
	{
	};

	TaskPtr acquireTask(unsigned id);
	//acquire task - select for update, returns custom unique pointer
	//update  task - save to db
	//release task - unlock (automatic)

	
	void bind( NamedPtr<KVBackend>& ptr, CLog& log);
	
	static const char* type_text;
};


//future
void bind( std::array<byte,2>& id, GroupCtl& group, CLog& log, short prefix, short len);
