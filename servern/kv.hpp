#include "Stream.cpp"

// leveldb DA
// gdbm
// lmdb DA /x -
// tokyo D
// kyoto DA - very slow compilation, no prefix search?
// upscaledb -
// we will have one backend for start that will be used for things like queues

struct KVBackendA
{
	/*
	virtual void Get(const CBuffer& key, CBuffer& val) =0;
	virtual void Set(const CBuffer& key, const CBuffer& val) =0;
	virtual void Del(const CBuffer& key) =0;
	virtual void GetFirst(CBuffer& key, CBuffer* val) =0;
	virtual void GetLast(CBuffer& key, CBuffer* val =0) =0;
	virtual void Commit() =0;
	*/
	virtual void Open(const char* name) =0;
	virtual ~KVBackendA();
};
