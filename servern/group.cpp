#include "group.hpp"

using std::string;;


struct COutput;
class GroupCtl;


	void GroupCtl::init(CLog& ilog, std::vector<t_config_database1>& cfgs)
	{
		log=ilog;
		main=0;
		for(t_config_database1& cfg : cfgs) {
			log("Opening database",cfg.name,cfg.enum_table[cfg.type],cfg.path);
			uptr<KVBackend> kv = OpenKV(cfg);
			cfg.kv= kv.get();
			auto rc = dbs.insert({cfg.name,move(kv)});
			if(!rc.second)
				throw std::runtime_error(tostring(log,"duplicate database name",cfg.name));
			if(!strcmp(cfg.name,"main")) {
				main=rc.first->second.get();
				//log("Have main database of type",cfg.enum_table[cfg.type]);
			}
		}
		if(!main)
			throw std::runtime_error(tostring(log,"no main database configured"));
		/* Load the ID map */
		CStUnStream<2> key;
		key.wb2(1<<14);
		CBufStream data = main->Get(key);
		IdMapVec.clear();
		if(data.left()) {
			while(data.left()) {
				IdMapElem el;
				data.rstrf(el.type_text);
				data.rstrf(el.name);
				el.id = data.r2();
				el.len= data.r2();
				//log("loaded idmap",el.type_text,el.name,el.id);
				IdMapVec.push_back(el);
			}
		} else {
			getID("-","idmap");
		}
	}

	KVBackend* GroupCtl::getKV(const immstring<16>& name)
	{
		auto it = dbs.find(name);
		if(it!=dbs.end()) {
			return it->second.get();
		} else {
			return 0;
		}
	}

	short GroupCtl::getID(const char* type_text, const char* name, byte len)
	{
		short next = 1<<14;
		for( const auto& el : IdMapVec ) {
			if( !strcmp(el.type_text,type_text) && !strcmp(el.name,name) ) {
				assert(el.len==len);
				LogKV("bound",type_text,name,"to",el.id);
				return el.id;
			}
			next = el.id + el.len;
		}
		IdMapElem el1 { type_text, name, next, len };
		IdMapVec.push_back(el1);
		/* Save the ID map */
		CStUnStream<2> key;
		key.wb2(1<<14);
		CStUnStream<2048> data; //todo: checked
		for( const auto& el : IdMapVec ) {
			data.wstrf(el.type_text);
			data.wstrf(el.name);
			data.w2(el.id);
			data.w2(el.len);
		}
		main->Set(key,data);
		LogKV("bound",type_text,name,"to",el1.id,"(new)");
		return el1.id;
	}

	void GroupCtl::Open()
	{
		std::lock_guard<std::mutex> lock (cs);
		if(!open_since.count()) {
			open_since = now();
		}
		nopen++;
	}

	void GroupCtl::Close()
	{
		std::unique_lock<std::mutex> lock (cs);
		if(nopen==1) {
			while((now()-open_since)<group_delay) {
				lock.unlock();
				std::this_thread::sleep_for(std::chrono::seconds(1));
				lock.lock();
			}
			ActuallyCommit();
			cv.notify_all();
		} else {
			nopen--;
			LogKV("wait for group commit leader");
			while(nopen) {
				cv.wait(lock);
			}
		}
	}
	void GroupCtl::ActuallyCommit()
	{
		LogKV("group commit");
		for(const auto& dbit : dbs) {
			dbit.second->Commit();
		}
	}

	//void GroupCtl::releaseTask(TTask *p);

	void GroupCtl::TaskPtr_deleter::operator()(TTask *p)
	{
		//todo
	};

	class TaskPtr
		: public std::unique_ptr<TTask, GroupCtl::TaskPtr_deleter >
	{
	};

	//TaskPtr GroupCtl::acquireTask(unsigned id);
	//acquire task - select for update, returns custom unique pointer
	//update  task - save to db
	//release task - unlock (automatic)
	
const char* GroupCtl::type_text = "database";

void GroupCtl::bind( NamedPtr<KVBackend>& ptr, CLog& log) {
	if(ptr.name.empty()) return;
	ptr.ptr= getKV(ptr.name);
	if(!ptr.ptr)
		throwNamedPtrNotFound(log, ptr.name, GroupCtl::type_text);
}

void bind( std::array<byte,2>& id, GroupCtl& group, CLog& log, short prefix, short len)
{
	assert(0);
}
