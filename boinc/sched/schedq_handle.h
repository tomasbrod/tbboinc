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

#include "sched_types.h"
#include <stdexcept>

void schedq_handle(SCHEDULER_REPLY& sreply);
bool schedq_handle_auth(SCHEDULER_REPLY& sreply);
void schedq_handle_results(SCHEDULER_REPLY& sreply);
void schedq_handle_lost(SCHEDULER_REPLY& sreply);
void schedq_handle_codesign(SCHEDULER_REPLY& sreply);

void schedq_handle_misc(SCHEDULER_REPLY& sreply);

bool schedq_reply_full(SCHEDULER_REPLY& sreply);

struct EDatabase	: std::runtime_error { using runtime_error::runtime_error; };

struct SCHED_QUEUE : DB_QUEUE {
	virtual void feed(SCHEDULER_REPLY& sreply) =0;
	explicit SCHED_QUEUE(const DB_QUEUE&) {};
	virtual ~SCHED_QUEUE() {};
};
