#include "COutput.hpp"
#include <cstddef>
#include <iomanip>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

using std::string;
using std::unique_ptr;

const char* COutput::type_text = "output";
const char* CPlugin::type_text = "plugin";

void COutput::init(CPlugin* iplugin, GroupCtl& group)
{
	plugin = iplugin;
	CLog log (type_text,name);
	ID=group.getID(this); //"output", 2, this->name, 0);
	ptrdb=0;
	for( auto& el : filesv ) {
		if(!el.isdir) {
			group.bind(el.db, log);
			if(!ptrdb) ptrdb = el.db.ptr;
			assert(el.path.empty());
		}
		else {
			assert(!el.path.empty());
			TUploadInfo upload = {
				.num=1,
				.taskid=125894,
			};
			log(getFileName(el, upload));
		}
	}
	if(!ptrdb) ptrdb = group.main;
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
	std::array<byte,16> cksum; // MD5 is 16 bytes
	static const size_t prefix;
	enum {
		Pointer=1,
		Complete=2,
		Trickle=4,
	};
};
const size_t TPointer::prefix = offsetof(TPointer,num);
#pragma pack(pop)

std::array<byte,16> hashMD5(const CBuffer& buf)
{
	return std::array<byte,16> {42,42,};
}

void COutput::saveOutput2( CLog& log1, TUploadInfo& upload, GroupCtl* group )
{
	if(upload.offset >= upload.size)
		throw EFileUpload("invalid upload offset");
	// find the location
	const t_config_subs_files* place = getPlace(upload);
	if(!place)
		throw EFileUploadPlace("no suitable destination");
	if(upload.offset >= upload.size)
		throw EFileUpload("invalid upload offset");
	CStUnStream<8> key = makeKey(upload);
	CUnchStream val1;
	const TPointer* ptr = 0;
	(place->isdir?ptrdb:place->db.ptr)->Get(key, val1);
	if(val1.length()) {
		assert(val1.length()>=TPointer::prefix); //db consistency
		ptr = (const TPointer*)val1.base;
		if( !(ptr->flags&TPointer::Trickle) != !upload.trickle )
			throw EFileUpload("file metadata does not match");
		if( ptr->flags&TPointer::Complete ) //fixme: eat the data and report success
			try { upload.stream->skip(upload.size-upload.offset); }
				catch(std::exception&) {}
	}
	if(place->isdir || ptr && (ptr->flags&TPointer::Pointer))
	{
		// is a pointer, or already started uploading as pointer
		if( !ptr || !(ptr->flags&TPointer::Pointer) ) {
			assert(val1.left() == sizeof(TPointer)); // db consistency
			CStUnStream<sizeof(TPointer)> val2;
			val2.w4(1234/*timestamp*/);
			val2.w1(TPointer::Pointer );
			val2.w1(upload.num);
			val2.w1(place->dirid);
			val2.w1(0);
			val2.w8(upload.size);
			val2.writea(upload.cksum);
			ptrdb->Set(key,val2);
		} else {
			if( (ptr->size!=upload.size) || (ptr->cksum!=upload.cksum) )
				throw EFileUpload("resume mismatched checksum or size");
			if(ptr->dirid!=place->dirid) for( auto& el : filesv ) {
				if( el.dirid == ptr->dirid && el.isdir ) {
					place = &el;
				}
			}
		}
		if(group) group->ReleaseNoCommit();
		CFileStream data;
		std::string filename = getFileName(*place,upload);
		try {
			data.openWrite(filename.c_str());
			//todo: whether to lock or not?
		} catch ( EFileAccess& e) {
			// file access error, probably already finalized
			if( !isFileComplete( filename, upload.size ) )
				throw ERecoverable(log1,"open upload file",e.what());
				else try { upload.stream->skip(upload.size-upload.offset); }
					catch(std::exception&) {}
		}
		if( upload.offset > data.length() )
			throw EFileUpload("resume offset leaves a hole");
		if(data.length() > upload.size)
			throw std::runtime_error("inconsistent file size");
		try {
			data.setpos(upload.offset);
			upload.stream->copyto(&data, upload.size-upload.offset);
			data.close();
			CFileStream::setReadOnly(filename.c_str(),true);
		} catch (EDiskFull& e) {
			throw ERecoverable(log1,"server storage full");
		} catch (std::system_error& e) {
			throw ERecoverable(log1,"uploading file",e.what());
		}
		if(group) group->Open();
	} else {
		if(group) group->ReleaseNoCommit();
		// or recv all and put to DB
		// if there is something in the db and the upload is of a resume
		size_t db_size = val1.left();
		/*if(upload.size<db_size)
			throw EFileUpload("resume truncates file");*/
		if(upload.offset>db_size)
			throw EFileUpload("resume offset leaves a hole");
		CBufStream data;
		data.allocate(TPointer::prefix + upload.size);
		TPointer* ptr = (TPointer*)data.getdata(TPointer::prefix,1);
		// copy from previous upload
		val1.skip(TPointer::prefix); // skip prefix
		val1.copyto(&data, upload.offset );
		assert(data.pos() == (upload.offset+TPointer::prefix)); //doublecheck
		// copy new data
		upload.stream->copyto(&data, upload.size-upload.offset);
		// set up metadata
		ptr->stamp = 1234;
		ptr->flags = 0 /*incomplete file*/;
		if( data.pos() == (TPointer::prefix+upload.size) )
			ptr->flags = TPointer::Complete /* complete file */;
		// check hash of the complete file
		if( hashMD5(CBuffer(data)+TPointer::prefix) != upload.cksum ) {
			place->db->Del(key);
			throw ERecoverable(log1,"checksum mismatch");
		}
		// finally put into database
		try {
			if(group) group->Open();
			place->db->Set(key, data);
		} catch (EDiskFull& e) {
			throw ERecoverable(log1,"server storage full");
		}
	}
}

void COutput::saveOutput( CLog& log1, TUploadInfo& upload )
{
	
	//todo: check if task state allows to upload of this file

	//todo: if a notice or unordered file, acquire a file number

	// unlock task
	// todo: if modified, save the task
	//task.reset();
	// lastly, enqueue task
	plugin->addValidate(upload.taskid, upload.num);
}

void COutput::saveReport( CLog& log1, GroupCtl::TaskPtr task, TReport& report, const std::string& tasklog)
{
	//serialize and save report
	//save log just like other output
}

bool COutput::isFileComplete(const std::string& filename, size_t size )
{
	struct ::stat statbuf;
	if(::stat(filename.c_str(),&statbuf)<0)
		throw std::system_error( errno, std::system_category());
	if( statbuf.st_mode & (S_IWUSR|S_IWGRP|S_IWOTH) )
		return false;
	return ( statbuf.st_size == size );
}

const t_config_subs_files* COutput::getPlace(const TUploadInfo& inf) const
{
	for( auto& el : filesv ) {
		if( inf.size > el.size ) continue;
		if( el.num!=-1 && inf.num != el.num ) continue;
		if( (inf.num==253 || inf.trickle) && !el.trash ) continue;
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

std::string COutput::getFileName(const t_config_subs_files& place, const TUploadInfo& inf) const
{
	std::stringstream ss;
	ss << place.path << "/"
	<< std::hex << std::setfill('0');
	if(place.group)
		ss << std::setw(2) << (inf.taskid&255) << "/";
	ss << std::setw(8) << inf.taskid
	<< std::setw(2) << inf.num;
	return ss.str();
}

//void COutput::saveReport( CLog& log1, GroupCtl::TaskPtr task, TReport& report, const std::string& tasklog);
// log like: host.192489.task.551456.postcard: 


#include "build/config-plugin.hpp"

// a taskID should be added to the validation queue, because something
// deemed inportant by <output> happened to it
void CPlugin::addValidate(unsigned id, short fileno)
{
	//lock
	CStUnStream<8> key;
	key.wb2(ID);
	key.wb4(tail);
	tail++;
	CStUnStream<8> val;
	key.w4(id);
	val.w1(fileno);
	//unlock
	kv->Set(key,val);
}


void CPlugin::init(GroupCtl& igroup)
{
	log=CLog(type_text,name);
	group= &igroup;
	kv= igroup.main;
	ID=group->getID(this,2); //"plugin", 1, this->name, 2);
	CStUnStream<8> key;
	key.wb2(ID);
	kv->GetLast(key);
	if(key.length()) {
		key.skip(2);
		tail=key.r4();
	}
	else tail=0;
	log("tail",tail);
	assert(TPointer::prefix==5);
	assert(sizeof(TPointer)==32);
}


