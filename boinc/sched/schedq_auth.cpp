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

// Handle scheduler authentication

#include "schedq_handle.h"

static bool schedq_auth_user(SCHEDULER_REPLY& sreply)
{
	char* authstr = strchr(sreply.request.authenticator, '_');
	//todo:check
	*(authstr++)=0;
	int userid = atoi(sreply.request.authenticator);
	//retval = user.lookup_id(userid);
	//todo:check
	// authenticator_v2 = userid + hash ( userid, server_key1 )
	// ^ only for scheduler RPC, unchanging for user, allows auth at stateless server
	//note: client handles missing fields (username/teamname) gracefully (for stateless)
	return false;
}

static void schedq_auth_host(SCHEDULER_REPLY& sreply)
{
	// lookup host, if something wrong, create a new one
	// always succeed or throw
	// also find_host_by_cpid
	//retval = h;
}

bool schedq_handle_auth(SCHEDULER_REPLY& sreply)
{
	//this deserves to be in separate file
	sreply.insert_message("Can't find host record", "low");
	return false;
	// step 1: lookup user based on id, and authenticate it with authenticator
	if(!schedq_auth_user(sreply))
		return false;
	// step 2: lookup host, check it, maybe create a new one
	schedq_auth_host(sreply);
}
