// This file is part of BOINC.
// http://boinc.berkeley.edu
// Copyright (C) 2019 University of California
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

#include "sched_vda.h"

#include "credit.h"
#include "sched_files.h"
#include "sched_main.h"
#include "sched_types.h"
#include "sched_util.h"
#include "handle_request.h"
#include "sched_msgs.h"
#include "sched_resend.h"
#include "sched_send.h"
#include "sched_config.h"
#include "sched_locality.h"
#include "sched_result.h"
#include "sched_customize.h"
#include "time_stats_log.h"

void process_request(char* code_sign_key);
void log_incomplete_request();
void log_user_messages();

void process_schedq_request(char* code_sign_key) {
	process_request(code_sign_key);
}
//STUB

void schedq_handle(SCHEDULER_REPLY& sreply)
{
	return; //STUB
}

void handle_schedq_request(FILE* fin, FILE* fout, char* code_sign_key) {
    SCHEDULER_REQUEST sreq;
    SCHEDULER_REPLY sreply;
    char buf[1024];

    g_request = &sreq;
    g_reply = &sreply;
    g_wreq = &sreply.wreq;

    sreply.nucleus_only = true;

    log_messages.set_indent_level(1);

    MIOFILE mf;
    XML_PARSER xp(&mf);
    mf.init_file(fin);
    const char* p = sreq.parse(xp);
    double start_time = dtime();
    if (!p){
        process_schedq_request(code_sign_key);

        if ((config.locality_scheduling || config.locality_scheduler_fraction) && !sreply.nucleus_only) {
            send_file_deletes();
        }
    } else {
        sprintf(buf, "Error in request message: %s", p);
        log_incomplete_request();
        sreply.insert_message(buf, "low");
    }

    if (config.debug_user_messages) {
        log_user_messages();
    }

    sreply.write(fout, sreq);
    log_messages.printf(MSG_NORMAL,
        "Scheduler ran %.3f seconds\n", dtime()-start_time
    );

    if (strlen(config.sched_lockfile_dir)) {
        unlock_sched();
    }
}

