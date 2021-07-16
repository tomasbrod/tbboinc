#include <memory>
#include "typese.hpp"
#include "Stream.hpp"
#include "kv.hpp"
#include "group.hpp"
#include "COutput.hpp"
#include "parse.hpp"
#include "tag.hpp"
#include "build/request.hpp"
#include "build/request.cpp"
#include <sodium/crypto_auth.h>

static unsigned char debase58[] = { 1, };

void split_file_name(unsigned long long& task, short& num, string& origin, const string& name)
{
	//tot4_21_F008000011_saran
	//origin is after last separator
	size_t sep2 = name.rfind('+');
	assert(sep2!=string::npos);
	origin = name.substr(sep2+1);
	assert(origin.size()<16);
	//num is last 1 base58 char
	assert(sep2>=2);
	char numc = name[sep2-1];
	assert(numc>=24&&numc<=127);
	num = debase58[ numc-24 ];
	//id, reverse separator search, 9 or 13 chars
	char ids[14];
	char* eptr;
	size_t sep1 = name.rfind('+',sep2-2);
	long len = sep2-sep1-2;
	assert(len==9||len==13);
	name.copy( ids, len, sep1+1 ); ids[len]=0;
	task= strtoull(ids,&eptr,16);
	assert(*eptr==0);
}

void check_file_sig(COutput::TUploadInfo& ui, t_file_upload_info& fu, const immstring<16> origin)
{
	CStUnStream<34> buf;
	buf.w1(COutput::type_id);
	buf.w1(ui.num);
	buf.w8(ui.taskid);
	buf.w8(fu.max_nbytes);
	buf.wstrf(origin);
	// hash verify config.server_keys.master
	// auth in fu.xml_signature
	//id.name[n]
	//id.keys
	//keys.name
	//keys.keys
}

#if 0
void handle_request(CHandleStream* in, CHandleStream* out, GroupCtl* group)
{
	CLog log ("handle_request");
	t_cgi_request_t req;
	{
		XML_PARSER2 xp (in);
		req.parse(xp);
	}
	assert(req.is_data);
	assert(req.data_server_request.is_upload);
	t_file_upload& fu = req.data_server_request.file_upload;
	COutput::TUploadInfo ui;
	string origin;
	split_file_name(ui.taskid, ui.num, origin, fu.file_info.name);
	assert(ui.taskid<4294967296);
	assert(ui.num<64);
	assert(origin==group->hostname);
	check_file_sig(ui, fu.file_info, fu.file_info.xml_signature);
	TTask task;
	loadTask(task, ui.taskid);
	group->bind(task.output);
	task.output->saveOutput(log, ui);
	group->Close();
	send_success();
	// implement task meta reading/id/dumping/restoring class
	// refactor the group commit
	// add thing holding hostnames and keys
	// this
}
#endif
