#pragma once
#include <memory>
#include "typese.hpp"
#include "Stream.hpp"
#include "build/config-db.hpp"

struct KVBackend
{
	virtual void Get(const CBuffer& key, CBuffer& val) =0;
	CBuffer Get(const CBuffer& key) { CBuffer val; Get(key,val); return val; } //fixme
	virtual void Set(const CBuffer& key, const CBuffer& val) =0;
	virtual void Del(const CBuffer& key) =0;
	//virtual void GetNext(CBuffer& key, CBuffer* val) =0;
	virtual void GetLast(CBuffer& key, CBuffer* val =0) =0;
	virtual void Commit() =0;
	virtual void Close() =0;
	virtual ~KVBackend();
	const t_config_database* cfg;
	protected:
	inline void on_destruct();
};

/* Open a database file according to config node */

std::unique_ptr<KVBackend> OpenKV(const t_config_database& cfg);

class CLog;
extern CLog LogKV;

// leveldb DA
// gdbm
// lmdb DA /x -
// tokyo D
// kyoto DA - very slow compilation, no prefix search?
// upscaledb -
// we will have one backend for start that will be used for things like queues
// group commit

struct t_config_database1 : t_config_database
{
	KVBackend* kv;
};
