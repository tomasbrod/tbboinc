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

struct FEEDER_SIMPLE : SCHED_QUEUE {
	FEEDER_SIMPLE(const DB_QUEUE& q)
		: SCHED_QUEUE(q)
	{
		//do nothing
/*
 * Parse the param xml to find which app to send (batch?, prio?)
 * Cache the application versions maybe?
*/
	}
	void feed(SCHEDULER_REPLY&)
	{
		//again, do nothing
		/* Find app version for the host. Start transaction.
		 * Fetch few results from db. FOR UPDATE
		 * Try to assign them.
		 * If assigned more than half, repeat query, else commit.
		 * Last, adjust user quota.
		*/
	}
};

SCHED_QUEUE* create_feeder_simple(DB_QUEUE& descr) { return new FEEDER_SIMPLE(descr); }
