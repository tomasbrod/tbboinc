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

struct IGroupObject
{
	virtual bool dump(XmlTag& xml, KVBackend* kv, short oid, CUnchStream& key, CUnchStream& val) =0;
	virtual void dump2(XmlTag& xml) =0;
	virtual void load(XmlParser& xml, GroupCtl* group) =0;
	virtual ~IGroupObject();
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
		union {
			unsigned hmask;
			IGroupObject* ptr;
		};
		std::vector<immstring<16>> ids;
		std::vector<IGroupObject*> ptrs;
	};
	std::array<KindMapElem,20> KindMapVec;

	const Ticks group_delay = Ticks(std::chrono::seconds(3));
	KVBackend* main;
	CLog log;
	std::mutex cs_nonce;
	uint64_t nonce_v, nonce_l;

	void init(CLog& ilog, std::vector<t_config_database1>& cfgs);

	KVBackend* getKV(const immstring<16>& name);

	short registerOID(const char* kind_text, unsigned kind, const char* name, unsigned hmask, IGroupObject* ptr);
	IGroupObject* getPtr(unsigned oid, unsigned kind=255);
	const char* getTypeText(unsigned oid, const char** name=0);
	template<class T> short registerOID(T* ptr, unsigned hmask=0) { return registerOID(T::type_text, T::type_id, ptr->name, hmask, ptr); }
	template<class T> T* getPtr(unsigned oid) { return (T*)getPtr(oid, T::type_id); }
	//TODO: allow registering non-object handlers (task,user...)

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
	void dumpxml(IStream& hs, bool skipped=false);
	void dumphex(IStream& hs, bool skipped=false);
	void load_pre(XmlParser& xp);
	void load(XmlParser& xp);
	void save_idmap();
	void save_nonce();

	uint64_t getNonce(unsigned long cnt = 1);
	
	static const char* type_text;
	static const unsigned type_id = 0;

};


//future
//void bind( std::array<byte,2>& id, GroupCtl& group, CLog& log, short prefix, short len);
