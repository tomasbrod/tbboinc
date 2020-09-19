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

// The BOINC scheduler.
// Normally runs as a CGI or fast CGI program.
// You can also run it:
// - manually for debugging, with a single request
// - for simulation or performance testing, with a stream of requests
//   (using --batch)

// TODO: what does the following mean?
// Also, You can call debug_sched() for whatever situation is of
// interest to you.  It won't do anything unless you create
// (touch) the file 'debug_sched' in the project root directory.
//

#include "boinc_fcgi.h"
#include "config.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cerrno>
#include <csignal>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcgiapp.h>

#include "boinc_db.h"
#include "error_numbers.h"
#include "filesys.h"
#include "parse.h"
#include "shmem.h"
#include "str_util.h"
#include "svn_version.h"
#include "synch.h"
#include "util.h"
#include "handle_request.h"
#include "sched_config.h"
#include "sched_files.h"
#include "sched_keyword.h"
#include "sched_msgs.h"
#include "sched_types.h"
#include "sched_util.h"

#include "sched_main.h"

// Useful for debugging, if your cgi script keeps crashing.  This
// makes it dump a core file that you can load into a debugger to see
// where the problem is.
#define DUMP_CORE_ON_SEGV 0

#define DEBUG_LEVEL 999
#define MAX_FCGI_COUNT 20

GUI_URLS gui_urls;
PROJECT_FILES project_files;
int g_pid;
static bool db_opened= false; // why?
bool batch= false;
bool mark_jobs_done= false;

static void usage( char* p )
{
	fprintf( stderr,
	         "Usage: %s [OPTION]...\n\n"
	         "Options:\n"
	         "  --batch            stdin contains a sequence of request messages.\n"
	         "                     Do them all, and ignore rpc_seqno.\n"
	         "  --mark_jobs_done   When send a job, also mark it as done.\n"
	         "                     (for performance testing)\n"
	         "  --debug_log        Write messages to the file 'debug_log'\n"
	         "  --simulator X      Start with simulated time X\n"
	         "                     (only if compiled with GCL_SIMULATOR)\n"
	         "  -h | --help        Show this help text\n"
	         "  -v | --version     Show version information\n",
	         p );
}

// call this only if we're not going to call handle_request()
//
static void send_message( MIOFILE& out, const char* msg, int delay )
{
	out.printf(
	         "Content-type: text/plain\n\n"
	         "<scheduler_reply>\n"
	         "    <message priority=\"low\">%s</message>\n"
	         "    <request_delay>%d</request_delay>\n"
	         "    <project_is_down/>\n"
	         "%s</scheduler_reply>\n",
	         msg, delay, config.ended ? "    <ended>1</ended>\n" : "" );
}

int open_database()
{
	// used in process_request()
	int retval;

	if( db_opened ) {
		retval= boinc_db.ping();
		if( retval ) {
			log_messages.printf( MSG_CRITICAL, "lost connection to database - trying to reconnect\n" );
		} else {
			return 0;
		}
	}

	retval= boinc_db.open( config.db_name, config.db_host, config.db_user, config.db_passwd );
	if( retval ) {
		log_messages.printf( MSG_CRITICAL, "can't open database\n" );
		return retval;
	}
	db_opened= true;
	return 0;
}

// If the scheduler 'hangs' (e.g. because DB is slow),
// Apache will send it a SIGTERM.
// Record this in the log file and close the DB conn.
//
void sigterm_handler( int /*signo*/ )
{
	if( db_opened ) {
		boinc_db.close();
	}
	log_messages.printf( MSG_CRITICAL, "Caught SIGTERM (sent by Apache); exiting\n" );
	//unlock_sched();
	fflush( (FILE*)NULL );
	exit( 1 );
	return;
}

static void log_request_headers( int& length )
{
	char* cl= getenv( "CONTENT_LENGTH" );
	char* ri= getenv( "REMOTE_ADDR" );
	char* rm= getenv( "REQUEST_METHOD" );
	char* ct= getenv( "CONTENT_TYPE" );
	char* ha= getenv( "HTTP_ACCEPT" );
	char* hu= getenv( "HTTP_USER_AGENT" );

	if( config.debug_request_details ) {
		log_messages.printf( MSG_NORMAL,
		                     "(req details) REQUEST_METHOD=%s CONTENT_TYPE=%s HTTP_ACCEPT=%s HTTP_USER_AGENT=%s\n", rm ? rm : "",
		                     ct ? ct : "", ha ? ha : "", hu ? hu : "" );
	}

	if( !cl ) {
		log_messages.printf( MSG_CRITICAL, "CONTENT_LENGTH environment variable not set\n" );
	} else {
		length= atoi( cl );
		if( config.debug_request_details ) {
			log_messages.printf( MSG_NORMAL, "CONTENT_LENGTH=%d from %s\n", length, ri ? ri : "[Unknown]" );
		}
	}
}

#if DUMP_CORE_ON_SEGV
void set_core_dump_size_limit()
{
	struct rlimit limit;
	if( getrlimit( RLIMIT_CORE, &limit ) ) {
		log_messages.printf( MSG_CRITICAL, "Unable to read resource limit for core dump size.\n" );
	} else {
		char short_string[256], *short_message= short_string;

		short_message+= sprintf( short_message, "Default resource limit for core dump size curr=" );
		if( limit.rlim_cur == RLIM_INFINITY ) {
			short_message+= sprintf( short_message, "Inf max=" );
		} else {
			short_message+= sprintf( short_message, "%d max=", (int)limit.rlim_cur );
		}

		if( limit.rlim_max == RLIM_INFINITY ) {
			short_message+= sprintf( short_message, "Inf\n" );
		} else {
			short_message+= sprintf( short_message, "%d\n", (int)limit.rlim_max );
		}

		log_messages.printf( MSG_DEBUG, "%s", short_string );

		// now set limit to the maximum allowed value
		limit.rlim_cur= limit.rlim_max;
		if( setrlimit( RLIMIT_CORE, &limit ) ) {
			log_messages.printf( MSG_CRITICAL, "Unable to set current resource limit for core dump size to max value.\n" );
		} else {
			log_messages.printf( MSG_DEBUG, "Set limit for core dump size to max value.\n" );
		}
	}
}
#endif

inline static const char* get_remote_addr()
{
	const char* r= getenv( "REMOTE_ADDR" );
	return r ? r : "?.?.?.?";
}

void schedq_handle(SCHEDULER_REPLY& sreply);

//static SCHED_MSG_LOG log_messages; //main thread log, TODO rename?
char* schedq_code_sign_key;

bool schedq_parse_file(SCHEDULER_REQUEST& srequest,MIOFILE& clientin,MIOFILE& clientout)
{
	//clientout is there for toclient-errors
	XML_PARSER xp(&clientin);
	const char* p = srequest.parse(xp);
	/*if (!p){
	process_schedq_request(code_sign_key);

	if ((config.locality_scheduling || config.locality_scheduler_fraction) && !sreply.nucleus_only) {
	send_file_deletes();
	}
	} else {*/
	if(p) {
    char buf[1024];
		sprintf(buf, "Error in request message: %s", p);
		//log_incomplete_request();
		send_message(clientout,buf,120);
		return false;
	}
	else return true;
	/*if (config.debug_user_messages) {
	log_user_messages();
	}*/
}

void* schedq_thread(void*) {
		//fork, loop, accept, parse, handle, write, flush
		//log goes to main stderr by default
		//if(debug), redirect log to dir, go via i/o files then maybe rename them
	FCGX_Request frequest;
	MIOFILE clinp, clout;
	SCHEDULER_REPLY sreply;
	SCHEDULER_REQUEST srequest;
	sreply.request= &srequest;
	FCGX_Init();
	FCGX_InitRequest( &frequest, 0, 0 );
	while( FCGX_Accept_r(&frequest) >= 0 ) {
		clinp.init_file(frequest.in );
		clout.init_file(frequest.out);
		if(schedq_parse_file(srequest,clinp,clout)) {
			sreply.log= &log_messages; //TODO
			sreply.db= &boinc_db; //TODO
			schedq_handle(sreply);
			sreply.write(stdout);
		}
		FCGX_Finish_r(&frequest);
	}
	return 0;
}

void schedq_init() {
	// Read Conig File
	int retval= config.parse_file();
	if( retval ) {
		log_messages.printf( MSG_CRITICAL,  "Can't parse config.xml: %s\n", boincerror( retval ) );
		exit( 0 );
	}
	srand( time( 0 ) + getpid() );
	log_messages.set_debug_level( config.sched_debug_level );
	if( config.sched_debug_level == 4 )
		g_print_queries= true;

	open_database();
	//gui_urls.init(); TODO!
	//project_files.init();
	//init_file_delete_regex();

	char path[MAXPATHLEN];
	sprintf( path, "%s/code_sign_public", config.key_dir );
	retval= read_file_malloc( path, schedq_code_sign_key );
	if( retval ) {
		log_messages.printf( MSG_CRITICAL, "Can't read code sign key file (%s)\n", path );
		exit( 0 );
	}
	strip_whitespace( schedq_code_sign_key );

}

int main( int argc, char** argv )
{
	int retval;
	char req_path[MAXPATHLEN], reply_path[MAXPATHLEN];
	char log_path[MAXPATHLEN];
	unsigned int counter= 0;
	int length= -1;
	log_messages.pid= getpid();
	bool debug_log= false;
	bool sched_stdio= false;

	/* Options
	 * --verbose - verbose logging
	 * --test - read stdin, write stdout, log stderr
	 * --batch ??
	 * --mark_jobs_done ??
	 * config.debug_req_reply_dir dump request, reply and log to the directory for each request
	 * BOINC_PROJECT_DIR - config file location
	*/

	for( int i= 1; i < argc; i++ ) {
		if( !strcmp( argv[i], "--batch" ) ) {
			batch= true;
			continue;
		} else if( !strcmp( argv[i], "--mark_jobs_done" ) ) {
			mark_jobs_done= true;
		} else if( !strcmp( argv[i], "--verbose" ) ) {
			debug_log= true;
		} else if( !strcmp( argv[i], "-h" ) || !strcmp( argv[i], "--help" ) ) {
			usage( argv[0] );
			exit( 0 );
		} else if( !strcmp( argv[i], "-v" ) || !strcmp( argv[i], "--version" ) ) {
			printf( "%s\n", SVN_VERSION );
			exit( 0 );
		} else if( !strcmp( argv[i], "--stdio" )  ) {
			debug_log= true;
			sched_stdio= true;
		} else if( strlen( argv[i] ) ) {
			log_messages.printf( MSG_CRITICAL, "unknown command line argument: %s\n\n", argv[i] );
			usage( argv[0] );
			exit( 1 );
		}
	}

	// install a signal handler that catches SIGTERMS sent by Apache if the CGI
	// times out.
	//
	signal( SIGTERM, sigterm_handler );

#if DUMP_CORE_ON_SEGV
	set_core_dump_size_limit();
#endif

	// Initialize shared scheduler data (config, feeders)
	schedq_init();
	if(debug_log) {
		fprintf(stderr, "schedq: verbose=1 , sched_stdio=%d , debug_req_reply_dir=%s\n",sched_stdio,config.debug_req_reply_dir?config.debug_req_reply_dir:"0");
	}

	if(sched_stdio) {
		// run scheduler in stdio mode
		MIOFILE clientin, clientout;
		SCHEDULER_REPLY sreply;
		SCHEDULER_REQUEST srequest;
		sreply.request= &srequest;
		sreply.log= &log_messages;
		sreply.db= &boinc_db; //TODO
		clientin.init_file(stdin);
		clientout.init_file(stdout);
		if(schedq_parse_file(srequest,clientin,clientout)) {
			schedq_handle(sreply);
			sreply.write(stdout);
			exit(0);
		}
		else exit(1);
	} else {
		//actuall multithreaded FCGI goes here
		return 69;
		schedq_thread(0);
	}

#if 0
	if( debug_log ) {
		if( !freopen( "debug_log", "w", stderr ) ) {
			fprintf( stderr, "Can't redirect stderr\n" );
			exit( 1 );
		}
	} else {
		char* stderr_buffer;
		if( get_log_path( path, "scheduler.log" ) == ERR_MKDIR ) {
			fprintf( stderr, "Can't create log directory '%s'  (errno: %d)\n", path, errno );
		}
		FCGI_FILE* f= FCGI::fopen( path, "a" );
		if( f ) {
			log_messages.redirect( f );
		} else {
			char buf[256];
			fprintf( stderr, "Can't redirect FCGI log messages\n" );
			sprintf( buf, "Server can't open log file for FCGI (%.217s)", path );
			send_message( buf, config.maintenance_delay );
			exit( 1 );
		}
		// install a larger buffer for stderr.  This ensures that
		// log information from different scheduler requests running
		// in parallel aren't intermingled in the log file.
		//
		if( config.scheduler_log_buffer ) {
			stderr_buffer= (char*)malloc( config.scheduler_log_buffer );
			if( !stderr_buffer ) {
				log_messages.printf( MSG_CRITICAL, "Unable to allocate stderr buffer\n" );
			} else {
				retval= setvbuf( f->stdio_stream, stderr_buffer, _IOFBF, config.scheduler_log_buffer );
				if( retval ) {
					log_messages.printf( MSG_CRITICAL, "Unable to change stderr buffering\n" );
				}
			}
		}
	}





	g_pid= getpid();
	// while(FCGI_Accept() >= 0 && counter < MAX_FCGI_COUNT) {
	while( FCGI_Accept() >= 0 ) {
		counter++;
		log_messages.set_indent_level( 0 );
		if( config.debug_request_headers ) {
			log_request_headers( length );
		}

		if( !debug_log && check_stop_sched() ) {
			send_message( "Project is temporarily shut down for maintenance", config.maintenance_delay );
			goto done;
		}

		/*if (!ssp) {
		                attach_to_feeder_shmem();
		}*/
		if( !ssp ) {
			send_message( "Server error: can't attach shared memory", config.maintenance_delay );
			goto done;
		}
		if( config.keyword_sched ) {
			keyword_sched_init();
		}

		if( strlen( config.debug_req_reply_dir ) ) {
			struct stat statbuf;
			// the code below is convoluted because,
			// instead of going from stdin to stdout directly,
			// we go via a pair of disk files
			// (this makes it easy to save the input,
			// and to know the length of the output).
			// NOTE: to use this, you must create group-writeable dirs
			// boinc_req and boinc_reply in the project dir
			//
			sprintf( req_path, "%s/%d_%u_sched_request.xml", config.debug_req_reply_dir, g_pid, counter );
			sprintf( reply_path, "%s/%d_%u_sched_reply.xml", config.debug_req_reply_dir, g_pid, counter );

			// keep an own 'log' per PID in case general logging fails
			// this allows to associate at least the scheduler request with the client
			// IP address (as shown in httpd error log) in case of a crash
			sprintf( log_path, "%s/%d_%u_sched.log", config.debug_req_reply_dir, g_pid, counter );
			fout= FCGI::fopen( log_path, "a" );
			if( !fout ) {
				log_messages.printf( MSG_CRITICAL, "can't write client log file %s\n", log_path );
				exit( 1 );
			}
			fprintf( fout, "PID: %d Client IP: %s\n", g_pid, get_remote_addr() );
			fclose( fout );

			log_messages.printf( MSG_DEBUG, "keeping sched_request in %s, sched_reply in %s, custom log in %s\n", req_path,
			                     reply_path, log_path );
			fout= FCGI::fopen( req_path, "w" );
			if( !fout ) {
				log_messages.printf( MSG_CRITICAL, "can't write request file\n" );
				exit( 1 );
			}
			copy_stream( stdin, fout );
			fclose( fout );
			stat( req_path, &statbuf );
			if( length >= 0 && ( statbuf.st_size != length ) ) {
				log_messages.printf( MSG_CRITICAL, "Request length %d != CONTENT_LENGTH %d\n", (int)statbuf.st_size, length );
			}

			fin= FCGI::fopen( req_path, "r" );
			if( !fin ) {
				log_messages.printf( MSG_CRITICAL, "can't read request file\n" );
				exit( 1 );
			}
			fout= FCGI::fopen( reply_path, "w" );
			if( !fout ) {
				log_messages.printf( MSG_CRITICAL, "can't write reply file\n" );
				exit( 1 );
			}

			handle_schedq_request( fin, fout, code_sign_key );
			fclose( fin );
			fclose( fout );
			fin= FCGI::fopen( reply_path, "r" );
			if( !fin ) {
				log_messages.printf( MSG_CRITICAL, "can't read reply file\n" );
				exit( 1 );
			}
			copy_stream( fin, stdout );
			fclose( fin );

			// if not contacted from a client, don't keep the log files
			/* not sure what lead to the assumption of a client setting
			         CONTENT_LENGTH, but it's wrong at least on our current
			         project / Apache / Client configuration. Commented out.
			if (getenv("CONTENT_LENGTH")) {
			        unlink(req_path);
			        unlink(reply_path);
			}
			*/

		} else {
			handle_request( stdin, stdout, code_sign_key );
			fflush( stderr );
		}
done:
		if( config.debug_fcgi ) {
			log_messages.printf( MSG_NORMAL, "FCGI: counter: %d\n", counter );
		}
		log_messages.flush();
	} // do()
	if( counter == MAX_FCGI_COUNT ) {
		fprintf( stderr, "FCGI: counter passed MAX_FCGI_COUNT - exiting..\n" );
	} else {
		fprintf( stderr, "FCGI: FCGI_Accept failed - exiting..\n" );
	}
	// when exiting, write headers back to apache so it won't complain
	// about "incomplete headers"
	fprintf( stdout, "Content-type: text/plain\n\n" );
#endif
	if( db_opened ) {
		boinc_db.close();
	}
}

// the following stuff is here because if you put it in sched_limit.cpp
// you get "ssp undefined" in programs other than cgi

void RSC_JOB_LIMIT::print_log( const char* rsc_name )
{
	log_messages.printf(
	  MSG_NORMAL, "[quota] %s: base %d scaled %d njobs %d\n", rsc_name, base_limit, scaled_limit, njobs );
}

void JOB_LIMIT::print_log()
{
	if( total.any_limit() )
		total.print_log( "total" );
	if( proc_type_limits[0].any_limit() )
		proc_type_limits[0].print_log( "CPU" );
	if( proc_type_limits[1].any_limit() )
		proc_type_limits[1].print_log( "GPU" );
}

void JOB_LIMITS::print_log()
{
	log_messages.printf( MSG_NORMAL, "[quota] Overall limits on jobs in progress:\n" );
	project_limits.print_log();
	for( unsigned int i= 0; i < app_limits.size(); i++ ) {
		if( app_limits[i].any_limit() ) {
			APP* app= 0; //ssp->lookup_app_name( app_limits[i].app_name );
			if( !app )
				continue;
			log_messages.printf( MSG_NORMAL, "[quota] Limits for %s:\n", app->name );
			app_limits[i].print_log();
		}
	}
}
