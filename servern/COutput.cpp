#include "COutput.hpp"
#include <cstddef>

using std::string;
using std::unique_ptr;


void COutput::init(CPlugin* iplugin, GroupCtl& group)
{
	plugin = iplugin;
	CLog log ("output",name);
	ID=group.getID("output", this->name, 1);
	ptrdb=0;
	group.bind(trickle_db, log);
	for( auto& el : filesv ) {
		if(!el.isdir) {
			group.bind(el.db, log);
			if(!ptrdb) ptrdb = el.db.ptr;
			assert(el.path.empty());
		}
		else assert(!el.path.empty());
	}
}

#pragma pack(push, 1)
struct TPointer
{
	LEuint32 stamp;
	byte flags;
	byte num;
	byte dirid;
	byte _pad;
	LEuint64 size;
	std::array<byte,32> cksum;
	static const size_t prefix;
};
const size_t TPointer::prefix = offsetof(TPointer,num);
#pragma pack(pop)
	

void COutput::saveOutput( CLog& log1, GroupCtl::TaskPtr task, TUploadInfo& upload)
{
	// check if task state allows to upload
	// acquire file number
	// save and unlock task
	// find the location
		assert(upload.size > upload.offset); //with or without offset?

	std::array<byte,32> cksum;
	//if a normal file is being uploaded:
	// find the location
	const t_config_subs_files* place = getPlace(upload);
	assert(place);
	// if location is a directory, read and write file pointer
	CStUnStream<8> key = makeKey(upload);
	if(place->isdir) {
		CUnchStream val1;
		ptrdb->Get(key, val1);
		if(val1.left() < sizeof(TPointer) ) {
			CStUnStream<sizeof(TPointer)> val2;
			val2.w4(1234/*timestamp*/);
			val2.w1(1 /*pointer*/ );
			val2.w1(upload.num);
			val2.w1(place->dirid);
			val2.w1(0);
			val2.w8(upload.size);
			val2.writea(cksum);
			ptrdb->Set(key,val2);
		} else {
			TPointer* ptr = (TPointer*)val1.base;
			assert(ptr->flags&1); //is pointer
			assert(ptr->cksum==cksum);
			assert(ptr->size==upload.size);
		}
	}
	// unlock task
	task.reset();
	// or recv all and put to DB
	if(!place->isdir) {
		// if there is something in the db and the upload is of a resume
		CUnchStream val1;
		place->db->Get(key, val1);
		if(upload.offset) {
			assert(upload.offset <= (val1.length()-TPointer::prefix));
		}
		// either file does not exist yet, or it is marked as incomplete
		if(val1.length()) {
			TPointer* ptr = (TPointer*)val1.getdata(TPointer::prefix,0);
			assert( ptr->flags&2 ); // marked as incomplete
			assert( (val1.length()-TPointer::prefix) <= upload.size ); // todo: size with or without offset?
		}
		else assert(upload.offset == 0);
		CBufStream data; // todo: allocate upload
		TPointer* ptr = (TPointer*)data.getdata(TPointer::prefix,1);
		// copy from previous upload
		val1.copyto(&data, upload.offset );
		assert(data.pos() == (upload.offset+TPointer::prefix));
		// copy new data
		upload.stream->copyto(&data, upload.size-upload.offset);
		// set up metadata
		ptr->stamp = 1234;
		ptr->flags = 2 /*incomplete file*/;
		if( data.pos() == (TPointer::prefix+upload.size) )
			ptr->flags = 0 /* complete file */;
		// finally put into database
		place->db->Set(key, data.release());
	}	else {
		CFileStream data;
		data.openWriteLock(getFileName(*place,upload));
		size_t esize = data.length();
		assert( upload.size >= esize );
		assert( upload.offset <= esize );
		data.setpos(upload.offset);
		upload.stream->copyto(&data, upload.size-upload.offset);
		data.close();
	}
	// lastly, enqueue task
	plugin->addValidate(upload.taskid, upload.num);
}

const t_config_subs_files* COutput::getPlace(const TUploadInfo& inf) const
{
	for( auto& el : filesv ) {
		if( inf.size > el.size ) continue;
		if( el.ix!=-1 && inf.num != el.ix ) continue;
		return &el;
	}
	return 0;
}

CStUnStream<8> COutput::makeKey(TUploadInfo& inf) const
{
	CStUnStream<8> key;
	byte idh = 0b11100000;
	key.w1(idh);
	key.wb4(inf.taskid);
	key.w1(inf.num);
	return key;
}

//void COutput::saveReport( CLog& log1, GroupCtl::TaskPtr task, TReport& report, const std::string& tasklog);
// log like: host.192489.task.551456.postcard: 


#include "build/config-plugin.hpp"

// a taskID should be added to the validation queue, because something
// deemed inportant by <output> happened to it
void CPlugin::addValidate(unsigned id, short fileno)
{
	CStUnStream<8> key;
	key.wb2(ID);
	key.w4(id);
	CStUnStream<8> val;
	val.w1(fileno);
	kv->Set(key,val);
}


void CPlugin::init(GroupCtl& igroup)
{
	log=CLog("plugin",name);
	group= &igroup;
	kv= igroup.main;
	ID=group->getID("plugin", this->name, 4);
	CStUnStream<8> key;
	key.wb2(ID);
	kv->GetLast(key);
	if(key.length()) {
		key.skip(2);
		tail=key.r4();
	}
	else tail=0;
	log("tail",tail);
	log("offsetof(dirid)",offsetof(TPointer, dirid),"of",sizeof(TPointer));
}


