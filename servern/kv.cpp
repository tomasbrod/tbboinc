#include "kv.hpp"
#include <stdio.h>
#include <string>
#include <cstring>
#include <lmdb.h>
#include <assert.h>
#include <stdexcept>
#include <memory>
#include "Stream.hpp"
#include "log.hpp"
// 0.4s
#include "leveldb/db.h"
#include "leveldb/write_batch.h"
#include "leveldb/cache.h"
#include "leveldb/filter_policy.h"
#include "leveldb/env.h"
// +0.01
//#include <kclangc.h>
// +0.02
#include <kchashdb.h>
#include <kcplantdb.h>
// +1.2s
//#include <tcutil.h>
//#include <tchdb.h>
//#include <tcbdb.h>
// +0.04s

using std::string;
using std::unique_ptr;

// Because C++ silently disallows calling virtual functions in
// destructors, it is necessary to override this destructor with
// the following function in all derived classes.
KVBackend::~KVBackend() {}

void KVBackend::on_destruct() {
	try {
		Close();
	}
	catch(std::exception& e) {
		LogKV.error(e, "at ~KVBackend()");
	}
	catch(...) {
		LogKV.warn("KVBackend::Close() failed horribly at ~KVBackend()");
	}
}

CLog LogKV ("db");

/* Tuning

* LevelDB
	- block size
	- memtable size
	- sstable size
	- cache size
	- bloom bits
	- checks
	- compression
* Kyoto cabinet
	- small, linear flag
	- buckets
	- memory map size
	- compression
	- defrag
	- page size
	- cache size
* LMDB
	- sync level (0 1 2)
	- memory map size (and max db size)
*/

static const size_t mega = 1048576;

class KV_Lightning : public KVBackend
{
	MDB_env *env;
	MDB_dbi dbi;
	MDB_txn* txn;
	MDB_cursor *cur;
	public:
	void Get(const CBuffer& key, CBuffer& val);
	void Set(const CBuffer& key, const CBuffer& val);
	void Del(const CBuffer& key);
	void GetFirst(CBuffer& key, CBuffer* val);
	void GetLast(CBuffer& key, CBuffer* val =0);
	void Commit();
	void Open(const t_config_database& cfg);
	void Close();
	~KV_Lightning() {on_destruct();}
};

void KV_Lightning::Open(const t_config_database& cfg)
{
	this->cfg = &cfg;
	if( mdb_env_create(&env) )
		throw std::runtime_error("LMDB initialization failed");
	mdb_env_set_mapsize(env, mega * cfg.mmap);
	unsigned int flags = MDB_NOSUBDIR | MDB_NOTLS;
	if( cfg.sync == 1 )
		flags |= MDB_NOMETASYNC;
	if( cfg.sync == 0 )
		flags |= MDB_NOSYNC;
	if( cfg.small == 0 ) // if small is set, readahead is enabled
		flags |= MDB_NORDAHEAD;
	int rc = mdb_env_open(env, cfg.path.c_str(), flags, 0660);
	if(rc)
		throw std::runtime_error(mdb_strerror(rc));
	if( rc= mdb_txn_begin(env, 0, 0, &txn) )
		throw std::runtime_error(mdb_strerror(rc));
	if( rc= mdb_dbi_open(txn, 0, 0, &dbi) )
		throw std::runtime_error(mdb_strerror(rc));
	if( rc= mdb_cursor_open(txn, dbi, &cur) )
		throw std::runtime_error(mdb_strerror(rc));
}

void KV_Lightning::Close()
{
	int rc;
	if(!env) return;
	mdb_cursor_close(cur);
	cur=0;
	mdb_txn_abort(txn);
	txn=0;
	mdb_env_close(env);
	env=0;
}

void KV_Lightning::GetFirst(CBuffer& key, CBuffer* val)
{
	assert(key.pos() >= 2);
	MDB_val mkey = {key.pos(),key.base};
	MDB_val mval;
	int rv = mdb_cursor_get(cur, &mkey, &mval, MDB_SET_RANGE);
	if( rv==MDB_NOTFOUND || mkey.mv_size<2 || memcmp(key.base,mkey.mv_data,2)) {
		key.reset(); return; }
	if(rv) abort();
	key.reset(mkey.mv_data,mkey.mv_size);
	if(val) val->reset(mval.mv_data,mval.mv_size);
}

void KV_Lightning::GetLast(CBuffer& key, CBuffer* val)
{
	assert(key.pos() == 2);
	assert(key.base[1] < 255);
	key.base[1] += 1;
	MDB_val mkey = {key.pos(),key.base};
	MDB_val mval;
	int rv = mdb_cursor_get(cur, &mkey, 0, MDB_SET_RANGE);
	if(rv==MDB_NOTFOUND) { key.reset(); return; }
	if(rv) abort();
	rv = mdb_cursor_get(cur, 0, 0, MDB_PREV);
	if(rv==MDB_NOTFOUND) { key.reset(); return; }
	if(rv) abort();
	rv = mdb_cursor_get(cur, &mkey, &mval, MDB_GET_CURRENT);
	if(rv) abort();
	if(mkey.mv_size<2 || memcmp(key.base,mkey.mv_data,2)) {
		key.reset(); return;
	} /* okay */
	key.reset(mkey.mv_data,mkey.mv_size);
	if(val) val->reset(mval.mv_data,mval.mv_size);
}

void KV_Lightning::Get(const CBuffer& key, CBuffer& val)
{
	MDB_val mkey = {key.pos(),key.base};
	MDB_val mval;
	int rv = mdb_get ( txn, dbi, &mkey, &mval);
	if(rv==MDB_NOTFOUND) { val.reset(); return; }
	val.reset(mval.mv_data,mval.mv_size);
}

void KV_Lightning::Set(const CBuffer& key, const CBuffer& val)
{
	MDB_val mkey = {key.pos(),key.base};
	MDB_val mval = {val.pos(),val.base};
	int rv = mdb_put ( txn, dbi, &mkey, &mval, 0);
	if(rv)
		throw std::runtime_error(mdb_strerror(rv));
}

void KV_Lightning::Del(const CBuffer& key)
{
	MDB_val mkey = {key.pos(),key.base};
	int rv = mdb_del ( txn, dbi, &mkey, NULL);
	if(rv!=MDB_NOTFOUND)
		throw std::runtime_error(mdb_strerror(rv));
}

void KV_Lightning::Commit()
{
	int rc;
	mdb_cursor_close(cur);
	cur=0;
	if( rc= mdb_txn_commit(txn) )
		throw std::runtime_error(mdb_strerror(rc));
	txn=0;
	if( rc= mdb_txn_begin(env, 0, 0, &txn) )
		throw std::runtime_error(mdb_strerror(rc));
	if( rc= mdb_cursor_open(txn, dbi, &cur) )
		throw std::runtime_error(mdb_strerror(rc));
}

class KV_Level : public KVBackend
{
	leveldb::DB* db;
	leveldb::Options options;
	//TODO: leveldb::WriteBatch batch;
	//FIXME: make atomic transactions somehow with read_uncommited
	leveldb::WriteOptions woptions;
	leveldb::ReadOptions roptions;
	~KV_Level() {on_destruct();}
	struct Logger : leveldb::Logger {
		CLog log;
		virtual void Logv(const char* format, std::va_list ap)
		{
			char buf[1024];
			char fmt2[256];
			size_t len = strlen(format);
			if(len && format[len-1]=='\n') {
				len--; if(len>=256) len=255;
				memcpy(fmt2, format, len);
				fmt2[len]=0;
				vsnprintf(buf,sizeof buf, fmt2, ap);
			}
			else vsnprintf(buf,sizeof buf, format, ap);
			log(buf);
		}
	};
	Logger logger;
	public:
	void Open(const t_config_database& cfg)
	{
		this->cfg = &cfg;
		leveldb::Options options;
		options.create_if_missing = true;
		#if 0
		logger.log= CLog(LogKV,cfg.name,"LevelDB");
		#else 
		logger.log= CLog(LogKV,cfg.name);
		#endif
		options.info_log = &logger;
		woptions.sync= cfg.sync>0;
		if(cfg.block)
			options.block_size = 1024ULL * cfg.block;
		if(cfg.memtable)
			options.write_buffer_size = mega * cfg.memtable;
		if(cfg.sstable)
			options.max_file_size = mega * cfg.sstable;
		if(cfg.cache) {
			options.block_cache = leveldb::NewLRUCache(cfg.cache * mega);
		}
		if(cfg.bloom) {
			options.filter_policy = leveldb::NewBloomFilterPolicy(cfg.bloom);
		}
		if(cfg.checksum)
			roptions.verify_checksums = 1;
		if(cfg.checksum>=2)
			options.paranoid_checks = 1;
		if(cfg.compress==0)
			options.compression = leveldb::kNoCompression;
		leveldb::Status status = leveldb::DB::Open(options, cfg.path, &db);
		if(!status.ok()) {
			delete options.filter_policy;
			delete options.block_cache;
			throw std::runtime_error("LevelDB open failed");
		}
	}
	void Commit()
	{
		// This is weird, but does a fsync
		leveldb::WriteBatch tmpbatch;
		leveldb::Status s = db->Write(woptions, &tmpbatch);
		if(!s.ok())
			throw std::runtime_error("LevelDB commit failed");
	}
	void Close() override
	{
		logger.log("KV_Level::Close()");
		delete db;
		delete options.block_cache;
		delete options.filter_policy;
	}
	void Get(const CBuffer& key, CBuffer& val)  {}
	void Set(const CBuffer& key, const CBuffer& val) {}
	void Del(const CBuffer& key) {}
	void GetFirst(CBuffer& key, CBuffer* val) {}
	void GetLast(CBuffer& key, CBuffer* val =0) {}
};

template <class DB> class KV_KyotoAny : public KVBackend
{
	protected:
  DB db;
  bool hard_txc;
	~KV_KyotoAny() {on_destruct();}
	struct Logger : kyotocabinet::BasicDB::Logger {
		CLog plog;
		virtual void log (const char *file, int32_t line, const char *func, Kind kind, const char *message)
		{
			if(kind>=WARN) {
				plog.warn(func,message);
			} else if(kind>=INFO) {
				plog(func,message);
			}
			#ifndef NDEBUG
			else {
				plog("Debug:",file,line,func,message);
			}
			#endif
		}
	};
	Logger logger;
	void Open1(const t_config_database& cfg)
	{
		this->cfg = &cfg;
		logger.plog= CLog(LogKV,cfg.name);
		db.tune_logger(&logger, Logger::WARN|Logger::ERROR|Logger::INFO
			#if 0
			|Logger::DEBUG
			#endif
		);
		if(cfg.mmap)
			db.tune_map( cfg.mmap * mega );
		hard_txc = cfg.sync > 0;
		int opts = 0;
		if(cfg.compress==1)
			opts |= kyotocabinet::HashDB::TCOMPRESS;
		if(cfg.small==1)
			opts |= kyotocabinet::HashDB::TSMALL;
		if(cfg.linear==1)
			opts |= kyotocabinet::HashDB::TLINEAR;
		if(opts)
			db.tune_options( opts );
		if(cfg.kyotofbp>0)
			db.tune_fbp( cfg.kyotofbp );
		if(cfg.buckets)
			db.tune_buckets( 1000ULL * cfg.buckets );
		if(cfg.defrag>0)
			db.tune_defrag( cfg.defrag );
	}
	void Open2(const t_config_database& cfg)
	{
		if (!db.open(cfg.path, db.OWRITER | db.OCREATE)) {
			throw std::runtime_error(db.error().name());
		}
		if(! db.begin_transaction(hard_txc) )
		{throw std::runtime_error(db.error().name());}
  }
	public:
	void Open(const t_config_database& cfg)
	{
		Open1(cfg);
		Open2(cfg);
	}
	void Close()
	{
		logger.plog("KV_KyotoAny::Close()");
		// Abort transaction
		if(! db.end_transaction(false) )
			throw std::runtime_error(db.error().name());
		if(!db.close()) {
			throw std::runtime_error(db.error().name());
		}
  }
  void Commit() {
		if(! db.end_transaction(true) )
			throw std::runtime_error(db.error().name());
		if(! db.begin_transaction(hard_txc) )
			throw std::runtime_error(db.error().name());
	}

	void Get(const CBuffer& key, CBuffer& val) {}
	void Set(const CBuffer& key, const CBuffer& val) {}
	void Del(const CBuffer& key) {}
	void GetFirst(CBuffer& key, CBuffer* val)
	{ throw std::runtime_error("unimplemented"); }
	void GetLast(CBuffer& key, CBuffer* val)
	{ throw std::runtime_error("unimplemented"); }
};

class KV_Kyoto : public KV_KyotoAny<kyotocabinet::TreeDB>
{
	public:
	void Open(const t_config_database& cfg)
	{
		Open1(cfg);
		if(cfg.block)
			db.tune_page( cfg.block * mega );
		if(cfg.cache)
			db.tune_page_cache( cfg.cache * mega );
		Open2(cfg);
	}
	void GetFirst(CBuffer& key, CBuffer* val)
	{
	}
	void GetLast(CBuffer& key, CBuffer* val =0)
	{
	}
};

class KV_KyotoHash : public KV_KyotoAny<kyotocabinet::HashDB>
{
};

template<class DB> static KVBackend* opencfg(const t_config_database& cfg)
{
	DB* db = new DB();
	db->Open(cfg);
	return db;
	// implement runtime_error with same message support like log has
	// log name construction like a log message
}

struct Config_database : t_config_database
{
	// Config_database should have a unique_ptr to the KVBackend
	unique_ptr<KVBackend> db;
};

KVBackend* OpenKV(const t_config_database& cfg)
{
	switch(cfg.type) {
		case cfg.v_lmdb:
			return opencfg<KV_Lightning>(cfg);
		case cfg.v_kyoto:
			return opencfg<KV_Kyoto>(cfg);
		case cfg.v_kyotohash:
			return opencfg<KV_KyotoHash>(cfg);
		case cfg.v_level:
			return opencfg<KV_Level>(cfg);
		default:
			throw std::runtime_error("OpenKV unimplemented");
	}
}

// https://github.com/google/leveldb/blob/master/doc/index.md
// https://dbmx.net/kyotocabinet/
// https://upscaledb.com/apis.html
// https://www.gnu.org.ua/software/gdbm/manual/index.html

#include "kv-grp.cpp"
