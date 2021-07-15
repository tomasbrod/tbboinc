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
	: t_config_group
{
	std::mutex cs;
	std::condition_variable cv;
	unsigned nopen;
	Ticks open_since;
	std::map<immstring<16>,unique_ptr<KVBackend>> dbs;

	struct KindMapElem {
		immstring<8> type_text;
		unsigned shift=255;
		std::vector<immstring<16>> ids;
		std::vector<void*> ptrs;
	};
	std::vector<KindMapElem> KindMapVec;

	const Ticks group_delay = Ticks(std::chrono::seconds(3));
	KVBackend* main;
	CLog log;
	std::mutex cs_nonce;
	uint64_t nonce_v, nonce_l;

	void init(CLog& ilog, std::vector<t_config_database1>& cfgs);

	KVBackend* getKV(const immstring<16>& name);

	short getID(const char* type_text, unsigned kind, const char* name, void* ptr, unsigned shift);
	void* getPtr(unsigned pos, unsigned kind);
	void* getPtrO(unsigned oid, unsigned kind);
	template<class T> short getID(T* ptr, unsigned shift=0) { return getID(T::type_text, T::type_id, ptr->name, ptr, shift); }
	//template<class T> T* getPtr(unsigned pos) { return (T*)getPtr(pos, T::type_id); }
	template<class T> T* getPtrO(unsigned oid) { return (T*)getPtrO(oid, T::type_id); }
	const char* getTypeText(unsigned oid, const char** name=0);

	void Open();

	void ReleaseNoCommit() {}
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

	void dump_id(XmlTag& parent);

	uint64_t getNonce(unsigned long cnt = 1);
	
	static const char* type_text;
	static const unsigned type_id = 0;
};


//future
void bind( std::array<byte,2>& id, GroupCtl& group, CLog& log, short prefix, short len);
