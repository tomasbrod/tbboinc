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

void schedq_invoke_feeders(SCHEDULER_REPLY& sreply)
{
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
