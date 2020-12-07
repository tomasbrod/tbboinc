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

// Simple result scheduler for CPU only.

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

#include "schedq_handle.h"

using std::runtime_error;

struct FEEDER_SIMPLE : SCHED_QUEUE {
	DB_APP app;
	FEEDER_SIMPLE(const DB_QUEUE& q)
		: SCHED_QUEUE(q)
	{
		/*
		 * Parse the param xml to find which app to send (batch?, prio?)
		 * Cache the application versions maybe?
		*/
    char attr_buf[256], app_name[256];
    MIOFILE mf;
    mf.init_buf_read(q.args);
		XML_PARSER xp(&mf);
		if(
			xp.get_tag(attr_buf, sizeof(attr_buf))
			|| xp.match_tag("wu")
			) throw runtime_error("no start tag");
		parse_attr(attr_buf, "app", app_name, sizeof(app_name));
		app.db=q.db;
		if(app.lookup_name(app_name))
			throw runtime_error("app not found");
	}
	int select_app_version(DB_APP_VERSION &version, SCHEDULER_REPLY& sreply) {
    char where [MAX_QUERY_LEN] = {0};
    std::map<DB_ID_TYPE, PLATFORM> platforms;
    DB_PLATFORM platform;
    snprintf(where, sizeof where, "WHERE name = '%s' limit 1", sreply.request.platform.name); //FIXME: escape
    if(0==platform.lookup(where)) {
			platforms.emplace(platform.id,platform);
		}
		for(const auto& cp : sreply.request.alt_platforms) {
			snprintf(where, sizeof where, "WHERE name = '%s' limit 1", cp.name);
			if(0==platform.lookup(where)) {
				platforms.emplace(platform.id,platform);
			}
		}
    snprintf(where, sizeof where, "WHERE appid = %lu and deprecated=0 order by version_num desc, id asc", app.id);
    while(0==version.enumerate(where)) {
			//not deprecated, min_core_version satisfied (TODO)
			if(version.deprecated || version.min_core_version>sreply.request.core_client_version)
				continue;
			if(version.version_num < app.min_version)
				continue;
			//no coproc support
			//patform matching supported one
			if(platforms.count(version.platformid)) {
				return 0;
			}
		}
		return 1;
	}
	bool assign_result(SCHEDULER_REPLY& sreply, DB_RESULT& result, DB_APP_VERSION& appver)
	{
		//check if assigned result of same non-zero wu
		if(result.workunitid) {
			long same_user_wus = 0;
			DB_RESULT result_counter;
			char where [MAX_QUERY_LEN] = {0};
			snprintf(where, sizeof where, "WHERE appid=%lu and workunitid=%lu and userid=%lu", app.id, result.workunitid, sreply.user.id);
			result_counter.count(same_user_wus, where);
			if(same_user_wus) {
				return false;
			}
		}
		return false;
		//send uniqe app and version
		//send workunit and 
	}
	void feed(SCHEDULER_REPLY& sreply)
	{
		//again, do nothing
		/* Find app version for the host. Start transaction.
		 * Fetch few results from db. FOR UPDATE
		 * Try to assign them.
		 * If assigned more than half, repeat query, else commit.
		 * Last, adjust user quota.
		*/
		// very simple version picking - enumerate backwards and pick first one that matches host platform
		// no coproc support yet
		DB_APP_VERSION version (sreply.db);
		if(select_app_version(version, sreply)) {
			message(sreply, "low", "No app version for app %s queue %s", app.name, this->name);
			return;
		}
		message(sreply, "low", "using app %s version %d #%lu", app.name, version.version_num,version.id);
    char where [MAX_QUERY_LEN] = {0};
		// assert in transaction
		DB_RESULT result (sreply.db);
		const unsigned select_limit = 10;
		g_print_queries=1;
    snprintf(where, sizeof where,
			"WHERE appid = %lu and server_state=%d limit %d FOR update" // skip locked
			, app.id
			, RESULT_SERVER_STATE_UNSENT
			, select_limit
		);
		while(1) {
			unsigned assigned = 0;
			while(0==result.enumerate(where)) {
				assigned += assign_result(sreply, result, version);
				if(schedq_reply_full(sreply))
					break;
			}
			if(assigned < select_limit/2) {
				break;
			}
		}
	}
};

SCHED_QUEUE* create_feeder_simple(DB_QUEUE& descr) { return new FEEDER_SIMPLE(descr); }
