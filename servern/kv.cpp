
#include <stdio.h>
#include <string>
#include <cstring>
#include <lmdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <assert.h>
#include <vector>
#include <stdexcept>
#include "Stream.cpp"
#include "kv.hpp"

using std::string;

// leveldb DA
// gdbm
// lmdb DA /x -
// tokyo D
// kyoto DA - very slow compilation, no prefix search?
// upscaledb -
// we will have one backend for start that will be used for things like queues

/* Going to use the Full DBA approach, because it is the most straightforward.
 * Not very flexible (cant abstract the k-v store), but okay.
 * and going to use lmdb
 */

struct TaskState
{
	long id;
	long plugin;
	char prefix[32]; // spt_la4 - app and batch name
	long server;
	long event_fileno;
	uint32_t tov_front;
	uint32_t tov_expire;
};

KVBackendA::~KVBackendA() {}

struct KVBackend : KVBackendA
{
	MDB_env *env;
	MDB_dbi dbi;
	MDB_txn* txn;
	MDB_cursor *cur;
	void Get(const CBuffer& key, CBuffer& val);
	void Set(const CBuffer& key, const CBuffer& val);
	void Del(const CBuffer& key);
	void GetFirst(CBuffer& key, CBuffer* val);
	void GetLast(CBuffer& key, CBuffer* val =0);
	void Commit();
	void Open(const char* name);
	~KVBackend();
};

KVBackend::~KVBackend() {}

class Database
{
	KVBackend* kv;
	uint32_t tail_task;
	std::vector<uint32_t> tail_tov;
	public:
	void open();
	void getTask(TaskState& task, long id);
	void getToValidate(TaskState& task, int& fileno);
	void deleteToValidate(const TaskState& task);
	void addToValidate(const TaskState& task, int fileno);
	~Database();
};

void KVBackend::Open(const char* name)
{
	if( mdb_env_create(&env) )
		throw std::runtime_error("LMDB initialization failed");
	int rc = mdb_env_open(env, name, MDB_NOSUBDIR | MDB_NORDAHEAD | MDB_NOTLS, 0660);
	if(rc)
		throw std::runtime_error(mdb_strerror(rc));
	if( rc= mdb_txn_begin(env, 0, 0, &txn) )
		throw std::runtime_error(mdb_strerror(rc));
	if( rc= mdb_dbi_open(txn, 0, 0, &dbi) )
		throw std::runtime_error(mdb_strerror(rc));
	if( rc= mdb_cursor_open(txn, dbi, &cur) )
		throw std::runtime_error(mdb_strerror(rc));
}

void KVBackend::GetFirst(CBuffer& key, CBuffer* val)
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

void KVBackend::GetLast(CBuffer& key, CBuffer* val)
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

void Database::addToValidate(const TaskState& task, int fileno)
{
	if(tail_tov.size()<task.plugin) tail_tov.resize(task.plugin,{0});
	if(!tail_tov[task.plugin]){
		CStUnStream<2> key;
		key.w1(task.plugin);
		key.w1(14);
		kv->GetLast(key);
		key.setpos(4);
		assert(key.left()==4);
		tail_tov[task.plugin]=key.r4();
	}
	CStUnStream<8> key;
	key.w1(task.plugin);
	key.w1(14);
	key.w4(tail_tov[task.plugin]);
	CStUnStream<2> data;
	data.w2(fileno);
	//kv->Set(key,data);
	// Set tov expire
}


void kv_test()
{
	KVBackend db;
	db.Open("test.db");
}
		
/*
* read task state
* read next queue item
* delete the queue item
* add queue item
* update task state with:
	* validator response (invalid/valid/postponed)
	* timeout extension
*/
