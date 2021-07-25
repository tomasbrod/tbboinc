#pragma once
#include <memory>
#include "typese.hpp"
#include "Stream.hpp"
#include "kv.hpp"
#include "group.hpp"

class CPlugin;

#include "build/config-output.hpp"

/* represents <output> element of config */
class COutput
	: public t_config_output
	, public IGroupObject
{
	/* Bound to plugin for event queue */
	CPlugin* plugin;
	short ID;
	KVBackend* ptrdb;

	public:
	static const char* type_text;
	static const unsigned type_id = 0;
  void init(CPlugin* iplugin, GroupCtl& group);

	//function to copy socket stream into file storage
	// save log file
	// update task status with error/success
	//
	struct TUploadInfo {
		short num;
		size_t size;
		size_t offset;
		IStream* stream;
		unsigned long long taskid;
		bool trickle;
		const TTask* task;
		std::array<byte,16> cksum;
	};
	void saveOutput( CLog& log1, TUploadInfo& upload);
	struct TReport {
		//...
	};
	void saveReport( CLog& log1, GroupCtl::TaskPtr task, TReport& report, const std::string& tasklog);
	private:
	const t_config_subs_files* getPlace(const TUploadInfo&) const;
	CStUnStream<8> makeKey(TUploadInfo&) const;
	std::string getFileName(const t_config_subs_files& place, const TUploadInfo& inf) const;
	void saveOutput2( CLog& log1, TUploadInfo& upload, GroupCtl* group );
	bool isFileComplete( const std::string& filename, size_t size );

	// IGroupObject
	virtual bool dump(XmlTag& xml, KVBackend* kv, short oid, CUnchStream& key, CUnchStream& val);
	virtual void dump2(XmlTag& xml);
	virtual void load(XmlParser& xml, GroupCtl* group);
};

struct EFileUpload : std::runtime_error
{
	using std::runtime_error::runtime_error;
};

struct EFileUploadPlace : EFileUpload
{
	using EFileUpload::EFileUpload;
};

#include "build/config-plugin.hpp"

/* represents <plugin> element of config */
class CPlugin
	: public t_config_plugin
	, public IGroupObject
{
	GroupCtl* group;
	KVBackend* kv;
	short ID;
	unsigned long tail;
	CLog log;
	// split the plugin into two parts: queues and listen
	// this thing manages the queues
	public:
	static const char* type_text;
	static const unsigned type_id = 1;

	// a taskID should be added to the validation queue, because something
	// deemed inportant by <output> happened to it
	void addValidate(unsigned id, short fileno);

	// add taskID to the expiration queue
	void setExpire(int id, int next);

	void init(GroupCtl& igroup);

	// IGroupObject
	virtual bool dump(XmlTag& xml, KVBackend* kv, short oid, CUnchStream& key, CUnchStream& val);
	virtual void dump2(XmlTag& xml);
	virtual void load(XmlParser& xml, GroupCtl* group);
};


