// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2020 University of California
// SchedQ: Ve.Brod
//
// BOINC is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// BOINC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with BOINC.  If not, see <http://www.gnu.org/licenses/>.

// Handle a scheduling server RPC

#include "config.h"
#ifdef _USING_FCGI_
#include "boinc_fcgi.h"
#else
#include <cstdio>
#endif
#include <cassert>
#include <cstdlib>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <cmath>

#include <unistd.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "backend_lib.h"
#include "boinc_db.h"
#include "error_numbers.h"
#include "filesys.h"
#include "parse.h"
#include "str_replace.h"
#include "str_util.h"
#include "util.h"

#include "sched_main.h"
#include "sched_types.h"
#include "sched_util.h"
#include "sched_msgs.h"
#include "sched_config.h"
#include "schedq_handle.h"

void process_request(char* code_sign_key);
void log_incomplete_request();
void log_user_messages();

struct SCHED_QUEUE : DB_QUEUE {
	virtual void feed(SCHEDULER_REPLY& sreply)=0;
	explicit SCHED_QUEUE(const DB_QUEUE&);
	virtual ~SCHED_QUEUE();
};

SCHED_QUEUE* schedq_get_queue(SCHEDULER_REPLY& sreply, DB_ID_TYPE queueid)
{
	//TODO: cache the queue descriptions, feeder instances and feeder plugins.
	DB_QUEUE descr(sreply.db);
	if(descr.lookup_id(queueid)) {
		sreply.log->printf(MSG_WARNING,"Queue %lu lookup failed\n",queueid);
		return 0;
	}
	//supposed to instantiate the feeder here, but unimplemented
	char buf[1024];
	snprintf(buf, sizeof(buf), "Error loading '%s' feeder for queue '%s' #%lu",
	descr.feeder, descr.name, descr.id );
	sreply.insert_message(buf, "low");
	return 0;
}

bool schedq_reply_full(SCHEDULER_REPLY& sreply)
{
	(void)sreply;
	return false; //TODO
}

void schedq_invoke_feeders(SCHEDULER_REPLY& sreply)
{
	//This is going to eventually enumerate queues, check user preferences,
	// load and instantiate feeders that are not loaded, or outdated
	// and invoke them. Porting the vanilla send_work is too much work.
	//So after the above is done, write a simple wu feeder.
	DB_SCHED_QUEUE_ITEM qpref (sreply.db);
	int retval;
	int count = 0;
	while( 0==(retval=qpref.enumerate_host(sreply.host, false)) ) {
		SCHED_QUEUE* queue = schedq_get_queue(sreply,qpref.id);
		if(!queue) continue;
		count++;
		queue->feed(sreply);
		if(schedq_reply_full(sreply)) break;
	}
	if(retval!=ERR_DB_NOT_FOUND)
		throw EDatabase("DB_SCHED_QUEUE_ITEM.enumerate failed");
	if(!count)
		sreply.insert_message("No useable queues", "low");
}

void schedq_handle(SCHEDULER_REPLY& sreply)
{

	bool auth_ok = schedq_handle_auth(sreply);
	if(!auth_ok) return;

	schedq_handle_cpid(sreply);
	schedq_handle_team(sreply);
	schedq_handle_urls(sreply);

	schedq_handle_results(sreply);

	//custom (shit like cpid, sticky files, in-progress wus, global_prefs, code_sign, msgs)

	schedq_invoke_feeders(sreply);

	return;
}
