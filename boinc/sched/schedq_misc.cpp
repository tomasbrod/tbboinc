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

// Misc scheduler handlers (cpid, team, usrl)

#include "schedq_handle.h"

void schedq_handle_misc(SCHEDULER_REPLY& sreply)
{
	// If new user CPID, update user record
	if (!sreply.request.using_weak_auth && sreply.request.cross_project_id[0]) {
		if (strcmp(sreply.request.cross_project_id, sreply.user.cross_project_id)) {
			safe_strcpy(sreply.user.cross_project_id, sreply.request.cross_project_id);
			// why mess with the DB here ??
			char buf[1024];
			char cross_project_id[1024];
			safe_strcpy(cross_project_id, sreply.request.cross_project_id);
			escape_string(cross_project_id, sizeof(cross_project_id));
			sprintf(buf, "cross_project_id='%.1004s'", cross_project_id);
			sreply.user.update_field(buf);
		}
	}

	// Load team credit and name
	if(sreply.user.teamid>0) {
		int retval = sreply.team.lookup_id(sreply.user.teamid);
		if(retval) {
			sreply.log->printf(MSG_WARNING, "Failed to lookup team %lu for user u%lu %s\n",
				sreply.user.teamid, sreply.user.id, boincerror(retval)
			);
		}
	}

	//TODO: urls, client stats, global_prefs
}
