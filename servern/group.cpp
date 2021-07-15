#include "group.hpp"
#include "parse4.hpp"

using std::string;;


struct COutput;
class GroupCtl;

struct IDumpable
{
	virtual void dump(XmlTag& xml, KVBackend* kv, short oid, CUnchStream& key, CUnchStream& val) =0;
	virtual void dump(XmlTag& xml) =0; //second pass dump
	virtual void load(XmlParser& xml, GroupCtl* group) =0;
};

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
		KindMapVec.clear();
		if(data.left()) {
			while(data.left()) {
				KindMapElem el;
				data.rstrf(el.type_text);
				el.shift=data.r1();
				size_t num = data.r1();
				el.ids.resize(num);
				el.ptrs.resize(num,0);
				for( auto& name : el.ids ) {
					data.rstrf(name);
					//log("loaded name",el.type_text,name);
				}
				//log("loaded KindMapElem",el.type_text,el.shift,el.ids.size());
				KindMapVec.push_back(el);
			}
		}
		short id = getID("group",0,"",this,8);
		assert(id=16384);
		key.skip(-2);
		key.wb2(16385);
		main->Get(key,data);
		if(data.left()<8)
			nonce_v=nonce_l=0;
		else
			nonce_v=nonce_l=data.r8();
		//log("nonce_l",nonce_l);
		//log("test nonce",getNonce());
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

	short GroupCtl::getID(const char* type_text, unsigned kind, const char* name, void* optr, unsigned shift)
	{
		assert(kind<8);
		if(kind>=KindMapVec.size()) {
			KindMapVec.resize(kind+1);
		}
		KindMapElem& km = KindMapVec[kind];
		if(km.shift==255) {
			km.type_text = type_text;
			km.shift = shift;
		} else {
			assert(!strcmp(km.type_text, type_text));
			assert(km.shift==shift);
		}
		unsigned fpos=255;
		for(unsigned pos=0; pos<km.ids.size(); ++pos) {
			if( !strcmp(km.ids[pos],name) ) {
				if(!km.ptrs[pos])
					km.ptrs[pos]= optr;
					else assert( km.ptrs[pos]==optr);
				short id = (kind<<10) | (1<<14) | (pos<<shift);
				if(kind) LogKV("bound",type_text,name,"to",id);
				return id;
			}
			else if(!km.ids[pos][0] && fpos<255)
				fpos= pos;
		}
		if(fpos==255) {
			fpos=km.ids.size();
			km.ids.push_back(name);
			km.ptrs.push_back(optr);
		} else {
			km.ids[fpos]= name;
			km.ptrs[fpos]=optr;
		}
		short id = (kind<<10) | (1<<14) | (fpos<<shift);
		LogKV("bound",type_text,name,"to",id,"(new) shl",shift);
		/* Save the ID map */
		CStUnStream<2> key;
		key.wb2(1<<14);
		CStUnStream<2048> data; //todo: checked
		for(unsigned kind=0; kind<KindMapVec.size(); ++kind) {
			const auto& el = KindMapVec[kind];
			data.wstrf(el.type_text);
			data.w1(el.shift);
			data.w1(el.ids.size());
			for(const auto& name : el.ids) {
				data.wstrf(name);
			}
		}
		main->Set(key,data);
		return id;
	}

	void* GroupCtl::getPtrO(unsigned oid, unsigned kind)
	{
		if( ((oid>>10)&7) != kind)
			return 0;
		assert(KindMapVec.size()>kind);
		KindMapElem& el = KindMapVec[kind];
		unsigned pos = ((oid&1023)>>el.shift) & 255;
		if(pos>=el.ptrs.size())
			return 0; //todo: throw?
		return el.ptrs[pos];
	}

	const char* GroupCtl::getTypeText(unsigned oid, const char** name)
	{
		unsigned kind = ((oid>>10)&7);
		if(name) *name=0;
		if(kind<KindMapVec.size()) {
			KindMapElem& el = KindMapVec[kind];
			unsigned pos = ((oid&1023)>>el.shift) & 255;
			if(name && (pos<el.ids.size()))
				*name = el.ids[pos];
			return KindMapVec[kind].type_text;
		}
		return 0;
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
		#if 1
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
		#else
		old_os = open_since;
		dirty=false;
		for(const auto& db : dbs)
			dirty |= db->dirty;
		while(dirty) {
			if(open_since!=old_os) break;
			if((now()-open_since)<group_delay)
				sleep(1);
			else {
				ActuallyCommit();
				dirty=false;
			}
		}
		open_since = 0;
		#endif
	}
	void GroupCtl::ActuallyCommit()
	{
		LogKV("group commit");
		for(const auto& dbit : dbs) {
			if(dbit.second.get()==main) continue;
			dbit.second->Commit();
		}
		//sync();
		main->Commit();
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

uint64_t GroupCtl::getNonce(unsigned long cnt)
{
	std::unique_lock<std::mutex> lock (cs_nonce);
	uint64_t ret = nonce_v++;
	if(ret>=nonce_l) {
		nonce_l+=16;
		CStUnStream<2> key;
		CStUnStream<8> val;
		key.wb2(16385);
		val.w8(nonce_l);
		main->Set(key,val);
		// force a commit under lock
	}
	assert(nonce_v); //overflow
	return ret;
}

const char* GroupCtl::type_text = "group";

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


void GroupCtl::dump_id(XmlTag& parent)
{
	XmlTag names (parent, "group" );
	names.attr("nonce").put((long long unsigned)nonce_l);
	for(unsigned kind=1; kind<KindMapVec.size(); ++kind) {
		auto& el = KindMapVec[kind];
		for(unsigned pos=0; pos<el.ids.size(); ++pos) {
			XmlTag tag (names, el.type_text);
			tag.attr("n").put(el.ids[pos]);
			short id = (kind<<10) | (1<<14) | (pos<<el.shift);
			tag.attr("i").put(id);
			if(el.shift)
				tag.attr("s").put((long)el.shift);
			tag.attr("p").put((long)pos);
		}
	}
}
