#pragma once
#include <memory>
#include "typese.hpp"
#include "Stream.hpp"
#include "kv.hpp"
#include "group.hpp"

class CPlugin;

#include "build/config-output.hpp"

/* represents <output> element of config */
class COutput : public t_config_output
{
	/* Bound to plugin for event queue */
	CPlugin* plugin;
	short ID;
	KVBackend* ptrdb;

	public:
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
		long taskid;
		immstring<33> cksum;
	};
	void saveOutput( CLog& log1, GroupCtl::TaskPtr task, TUploadInfo& upload);
	struct TReport {
		//...
	};
	void saveReport( CLog& log1, GroupCtl::TaskPtr task, TReport& report, const std::string& tasklog);
	private:
	const t_config_subs_files* getPlace(const TUploadInfo&) const;
	CStUnStream<8> makeKey(TUploadInfo&) const;
};

#include "build/config-plugin.hpp"

/* represents <plugin> element of config */
class CPlugin : public t_config_plugin
{
	GroupCtl* group;
	KVBackend* kv;
	short ID;
	unsigned long tail;
	CLog log;
	// split the plugin into two parts: queues and listen
	// this thing manages the queues
	public:

	// a taskID should be added to the validation queue, because something
	// deemed inportant by <output> happened to it
	void addValidate(unsigned id, short fileno);

	// add taskID to the expiration queue
	void setExpire(int id, int next);

	void init(GroupCtl& igroup);

};


