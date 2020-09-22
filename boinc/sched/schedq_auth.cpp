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

static bool invalid_authenticator(SCHEDULER_REPLY& sreply)
{
	sreply.insert_message(
		//todo: translate
		"Invalid or missing account key.  To fix, remove and add this project.",
		"notice");
	return false;
}

static bool schedq_auth_user(SCHEDULER_REPLY& sreply)
{
	sreply.user.db = sreply.db;
	char* authstr = strchr(sreply.request.authenticator, '_');
	if(!authstr) return invalid_authenticator(sreply);
	*(authstr++)=0;
	sreply.user.id = atoi(sreply.request.authenticator);
	int retval = sreply.user.lookup_id(sreply.user.id);
	if(retval==ERR_DB_NOT_FOUND) return invalid_authenticator(sreply);
	if(retval) throw EDatabase("lookup user failed");
	if(strcmp(authstr, sreply.user.authenticator)) return invalid_authenticator(sreply);
	// authenticator_v2 = userid + hash ( userid, server_key1 )
	// ^ only for scheduler RPC, unchanging for user, allows auth at stateless server
	//note: client handles missing fields (username/teamname) gracefully (for stateless)
	// for stateless, host authentication is quite impossible
	sreply.request.using_weak_auth = 0; //TODO
	return true;
}

static bool schedq_auth_host(SCHEDULER_REPLY& sreply)
{
	// lookup host, if something wrong, return false to create a new one
	// also find_host_by_cpid ??
	sreply.host.db = sreply.db;
	if(!sreply.request.hostid) return false;
	int retval = sreply.host.lookup_id(sreply.request.hostid);
	if(retval==ERR_DB_NOT_FOUND) {
		sreply.insert_message("Host not found", "low");
		return false;
	}
	if(retval) throw EDatabase("lookup host failed");
	// Check that host record matches the request details
	if (sreply.host.userid != sreply.user.id) {
		sreply.insert_message("Inconsistent host's user id", "low");
		return false;
	}
	if (sreply.request.rpc_seqno < sreply.host.rpc_seqno) {
		sreply.insert_message("RPC sequence number mismatch", "low");
		return false;
	}
	if( sreply.host.host_cpid[0] ) {
		char buf[1024], host_cpid_hash[256];
    sprintf(buf, "%s%s", sreply.request.host.host_cpid, sreply.user.email_addr);
    md5_block((const unsigned char*)buf, strlen(buf), host_cpid_hash);
    if(strcmp(host_cpid_hash, sreply.host.host_cpid)) {
			sreply.insert_message("Host CPID mismatch", "low");
			return false;
		}
	}
	// TODO: what if the request seqno is massively higher?
	return true;
}

static void schedq_auth_new_host(SCHEDULER_REPLY& sreply)
{
	// always succeed or throw
	sreply.host = sreply.request.host;
	sreply.host.db = sreply.db;
	sreply.host.id = 0;
	sreply.host.expavg_time = sreply.host.create_time = time(0);
	sreply.host.userid = sreply.user.id;
	sreply.host.rpc_seqno = 0;
	safe_strcpy(sreply.host.venue, sreply.user.venue);
	sreply.host.fix_nans();

	int retval = sreply.host.insert();
	if (retval) {
		sreply.db->print_error("host.insert()");
		throw EDatabase("insert host failed");
	}
	sreply.host.id = sreply.db->insert_id();

  // this tells client to updates its host ID and reset RPC seqno
	sreply.hostid = sreply.host.id;
	sreply.request.rpc_seqno = 0; // why update request?
	return;
}


void schedq_handle_cpid(SCHEDULER_REPLY& sreply)
{
	// compute email hash
	//
	md5_block(
		(unsigned char*)sreply.user.email_addr,
		strlen(sreply.user.email_addr),
		sreply.email_hash
	);
	//what if it does not match, and why cant we use sreply.user.email_hash?

	// if new user CPID, update user record
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
}

bool schedq_handle_auth(SCHEDULER_REPLY& sreply)
{
	// step 1: lookup user based on id, and authenticate it with authenticator
	if(!schedq_auth_user(sreply))
		return false;
	// step 2: lookup host, check it, maybe create a new one
	if(!schedq_auth_host(sreply))
		schedq_auth_new_host(sreply);
	schedq_handle_cpid(sreply);
	return true;
}
