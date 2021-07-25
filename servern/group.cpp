#include "group.hpp"
#include "parse.hpp"
#include "tag.hpp"
#include "build/dump.def.hpp"
#include "build/dump.def.cpp"

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
		for(auto& el : KindMapVec) {
			el.type_text.clear();
			el.ids.clear();
			el.ptrs.clear();
			el.ptr=0;
		}
		for(unsigned kind=0; data.left(); ++kind) {
				KindMapElem &el = KindMapVec[kind];
				data.rstrf(el.type_text);
				el.hmask=data.r1();
				size_t num = data.r1();
				el.ids.resize(num);
				el.ptrs.resize(num,0);
				for( auto& name : el.ids ) {
					data.rstrf(name);
					//log("loaded name",el.type_text,name);
				}
				//log("loaded KindMapElem",el.type_text,el.ids.size());
		}
		//todo: catch EStreamOutOfBounds and explain it correctly
		//note: group/idmap does not have to be registered, we can overlay COutput on the same slot
		key.skip(-2);
		key.wb2(16385);
		main->Get(key,data);
		if(data.left()<8)
			nonce_v=nonce_l=0;
		else
			nonce_v=nonce_l=data.r8();
		log("nonce_l",nonce_l);
		log("test nonce",getNonce());
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

	short GroupCtl::registerOID(const char* type_text, unsigned kind, const char* name, unsigned hmask, IGroupObject* optr)
	{
		assert(kind<8);
		KindMapElem& km = KindMapVec[kind];
		if(km.type_text.empty()) {
			km.type_text = type_text;
			km.hmask = hmask;
		}
		if(km.hmask!=hmask or strcmp(km.type_text, type_text)) {
			throw std::runtime_error(tostring(log,"mismatch while registering",kind,"as",type_text,hmask,"into",km.type_text,km.hmask));
		}
		assert(!strcmp(km.type_text, type_text));
		assert(km.hmask==hmask);
		unsigned fpos=255;
		for(unsigned pos=0; pos<km.ids.size(); ++pos) {
			if( !strcmp(km.ids[pos],name) ) {
				if(!km.ptrs[pos])
					km.ptrs[pos]= optr;
					else assert( km.ptrs[pos]==optr);
				short id = (kind<<10) | (1<<14) | (pos);
				LogKV("bound",type_text,name,"to",id);
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
		short id = (kind<<10) | (1<<14) | (fpos);
		LogKV("bound",type_text,name,"to",id,"(new)");
		save_idmap();
		return id;
	}
	void GroupCtl::save_idmap()
	{
		/* Save the ID map */
		CStUnStream<2> key;
		key.wb2(1<<14);
		CStUnStream<1048576> data; //todo: checked
		for(unsigned kind=0; kind<KindMapVec.size(); ++kind) {
			const auto& el = KindMapVec[kind];
			data.wstrf(el.type_text);
			data.w1(el.hmask);
			data.w1(el.ids.size());
			for(const auto& name : el.ids) {
				data.wstrf(name);
			}
		}
		main->Set(key,data);
	}

	IGroupObject* GroupCtl::getPtr(unsigned oid, unsigned kind)
	{
		if( kind!=255 && ((oid>>10) & 0b101111) != kind )
			return 0;
		assert(kind==255 || KindMapVec.size()>kind);
		kind = (oid >> 10) & 15;
		if(kind>KindMapVec.size())
			return 0;
		KindMapElem& el = KindMapVec[kind];
		unsigned pos = oid & 255;
		if(kind==0) pos= 0;
		if((oid>>8) & 3 > el.hmask)
			return 0;
		if(pos>=el.ptrs.size())
			return 0; //todo: throw?
		return el.ptrs[pos];
	}

	const char* GroupCtl::getTypeText(unsigned oid, const char** name)
	{
		unsigned kind = ((oid>>10)&15);
		if(name) *name=0;
		if(kind<KindMapVec.size()) {
			KindMapElem& el = KindMapVec[kind];
			unsigned pos = oid & 255;
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
		save_nonce();
	}
	assert(nonce_v); //overflow
	return ret;
}
void GroupCtl::save_nonce()
{
		CStUnStream<2> key;
		CStUnStream<8> val;
		key.wb2(16385);
		val.w8(nonce_l);
		main->Set(key,val);
		//todo: force a commit under lock
		//fixme: commit must not be forced, instead save this in standard group commit
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
	for(unsigned kind=0; kind<KindMapVec.size(); ++kind) {
		auto& el = KindMapVec[kind];
		for(unsigned pos=0; pos<el.ids.size(); ++pos) {
			if(el.ids[pos].empty()) continue;
			XmlTag tag (names, el.type_text);
			short id = (kind<<10) | (1<<14) | (pos);
			tag.attr("n").put(el.ids[pos]);
			tag.attr("i").put((long)id);
			tag.attr("p").put((long)pos);
			if(el.hmask)
				tag.attr("m").put((long)el.hmask);
		}
	}
}

IGroupObject::~IGroupObject() {}

void GroupCtl::dumpxml(IStream& hs, bool skipped)
{
	const size_t cols = 32;
	XmlTag doc ( &hs, "dump" );
	if(!skipped) {
		this->dump_id(doc);
	}
	for(const auto& dbit : dbs) {
		KVBackend* kv= dbit.second.get();
		if(kv->cfg->dump == skipped) {
			XmlTag skip(doc,"skipped");
			skip.attr("db").put(dbit.first);
			continue;
		}
		std::unique_ptr<KVBackend::Iterator> it = kv->getIterator();
		CUnchStream key, val;
		while(it->Get(key,val)) {
			long oid = key.rb2();
			if (oid<=0x40FF) continue;
			bool dumped=0;
			if(IGroupObject* obj = getPtr(oid))
			{
				dumped=obj->dump(doc, kv, oid, key, val);
			}
			if(!dumped)
			{
				XmlTag tag( doc, "unknown" );
				tag.attr("db").put(dbit.first);
				const char* type_name;
				const char* type_text = getTypeText(oid,&type_name);
				if(type_text)
					tag.attr("kind").put(type_text);
				if(type_name && *type_name)
					tag.attr("name").put(type_name);
				tag.attr("key");
				key.skip(-2);
				hs.writehex(key);
				hs.w1(' ');
				tag.in_tag=2;
				tag.body();
				while(val.length()>cols) {
					hs.w1('\n');
					hs.writehex(val.base,cols);
					val.base+=cols;
				}
				hs.writehex(val);
			}
		}
	}
	if(!skipped) {
		for(unsigned kind=1; kind<KindMapVec.size(); ++kind) {
			auto& el = KindMapVec[kind];
			for(unsigned pos=0; pos<el.ids.size(); ++pos) {
				if(el.ptrs[pos]) {
					el.ptrs[pos]->dump2(doc);
				}
			}
		}
	}
}

void GroupCtl::dumphex(IStream& hs, bool skipped)
{
	const size_t cols = 32;
	for(const auto& dbit : dbs) {
		KVBackend* kv= dbit.second.get();
		XmlTag tagdb( &hs, string(dbit.first) );
		if(kv->cfg->dump == skipped) {
			tagdb.attr("skipped").put_bool(1);
			continue;
		}
		std::unique_ptr<KVBackend::Iterator> it = kv->getIterator();
		CUnchStream key, val;
		while(it->Get(key,val)) {
			tagdb.body(1);
			hs.w1('[');
			hs.writehex(key);
			hs.w1(']');
			if((val.length()+key.length()+2)>cols)
				hs.w1('\n');
			while(val.length()>cols) {
				hs.writehex(val.base,cols);
				val.base+=cols;
				hs.w1('\n');
			}
			hs.writehex(val);
		}
	}
}

void GroupCtl::load_pre(XmlParser& xp)
{
	xp.get_tag();
	xp.get_tag();
	if(0==strcmp(xp.tag,"group")) {
		t_dump_group st;
		st.parse(xp);
		nonce_v=nonce_l= st.nonce;
		for(auto& el : KindMapVec) {
			el.type_text.clear();
			el.ids.clear();
			el.ptrs.clear();
			el.ptr=0;
		}
		for(auto& el : st.entry) {
			unsigned kind = ((el.i>>10)&15);
			unsigned pos = el.i&255;
			KindMapVec[kind].type_text=el.k;
			KindMapVec[kind].hmask=el.m;
			if(pos>=KindMapVec[kind].ids.size()) {
				KindMapVec[kind].ids.resize(pos+1);
				KindMapVec[kind].ptrs.resize(pos+1,NULL);
			}
			KindMapVec[kind].ids[pos]=el.n;
			KindMapVec[kind].ptrs[pos]=NULL;
		}
		for(const auto& dbit : dbs) {
			if(dbit.second->cfg->dump) {
				dbit.second->WipeClean();
				log.warn("Wiped",dbit.second->cfg->name);
			}
		}
		save_idmap();
		save_nonce();
	}
	else {
		throw EXmlParse(xp,false,"group");
	}
}

void GroupCtl::load(XmlParser& xp)
{
	while(xp.get_tag()) {
		// find kind matching the xp.tag
		unsigned kind = 20;
		for(kind=0; kind<20; ++kind) {
			if(0==strcmp(KindMapVec[kind].type_text,xp.tag))
				break;
		}
		if(/*it is object*/ kind < 16 ) {
			xp.get_attr();
			assert(0==strcmp("n",xp.attr));
			immstring<16> name;
			xp.get_str(name,16);
			//find the object name
			IGroupObject* obj=0;
			for(unsigned pos=0; pos<KindMapVec[kind].ids.size(); ++pos) {
				if(0==strcmp(KindMapVec[kind].ids[pos],name)) {
					obj=KindMapVec[kind].ptrs[pos];
					break;
				}
			}
			if(/*found object ptr*/obj) {
				obj->load(xp,this);
			} else {
				log.warn("Found orphan object",xp.tag,name);
				xp.skip();
			}
		} else if( /*it is not object*/ kind < 20 ) {
			IGroupObject* obj= KindMapVec[kind].ptr;
			if(obj) {
				obj->load(xp,this);
			} else {
				log.warn("Found orphan special",xp.tag);
				xp.skip();
			}
		}
		else if(0==strcmp("unknown",xp.tag)) {
			std::unique_ptr<t_dump_unknown> st ( new t_dump_unknown );
			st->parse(xp);
			auto kv = getKV(st->db);
			if(kv) {
				size_t klen = strlen(st->key) / 2;
				size_t vlen = strlen(st->val) / 2;
				bool dec = CBuffer::hex2bin(
					(byte*)static_cast<char*>(st->key),
					static_cast<const char*>(st->key),
					klen*2);
				dec &= CBuffer::hex2bin(
					(byte*)static_cast<char*>(st->val),
					static_cast<const char*>(st->val),
					vlen*2);
				if(dec) {
					kv->Set(
						CBuffer(static_cast<char*>(st->key), klen),
						CBuffer(static_cast<char*>(st->val), vlen)
					);
				}
				else log.warn("Unable to decode unknown data tag for",st->db);
			}
			else log.warn("Unable resolve unknown data tag for",st->db);
		} else {
			log.warn("Found unknown tag",xp.tag);
			xp.skip();
		}
	}
}
