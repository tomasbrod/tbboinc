/* Clean up old workunits and results */
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <bitset>
#include <ctime>
#include <unistd.h>
#include <cstdlib>
#include <sys/stat.h>

#include "odlkcommon/common.h"
#include "odlkcommon/namechdlk10.h"
#include "family_mar/prov_blk_trans.h"

#include "boinc_api.h"
#include "bocom/Stream.cpp"

#include "config.h"
#include "backend_lib.h"
#include "error_numbers.h"
#include "sched_config.h"
#include "sched_util.h"

#include "odlkcommon/namechdlk10.cpp"
#include "odlkcommon/kvio.cpp"

struct EDatabase	: std::runtime_error { using runtime_error::runtime_error; };
struct EInvalid	: std::runtime_error { using runtime_error::runtime_error; };
static int retval;

#include "wio.cpp"

void initz() {
	int retval = config.parse_file();
	if (retval) {
			log_messages.printf(MSG_CRITICAL,
					"Can't parse config.xml: %s\n", boincerror(retval)
			);
			exit(1);
	}

	retval = boinc_db.open(
			config.db_name, config.db_host, config.db_user, config.db_passwd
	);
	if (retval) {
			log_messages.printf(MSG_CRITICAL,
					"boinc_db.open failed: %s\n", boincerror(retval)
			);
			exit(1);
	}
}

bool f_write;

int delete_workunit_files(DB_WORKUNIT& wu)
{
	char path[MAXPATHLEN];
	char path_gz[MAXPATHLEN], path_md5[MAXPATHLEN];
	bool no_delete=false;
	bool errored= false;

	MIOFILE mf;
	mf.init_buf_read(wu.xml_doc);
	XML_PARSER xp(&mf);

	while (!xp.get_tag()) {
		if (!xp.is_tag) continue;
		if (xp.match_tag("file_info")) {
			string filename;
			no_delete = false;
			while (!xp.get_tag()) {
				if (xp.parse_string("name", filename)) {
					continue;
				} else if (xp.parse_bool("no_delete", no_delete)) {
					continue;
				} else if (xp.match_tag("/file_info")) {
					break;
				}
			}
			if (!xp.match_tag("/file_info") || filename.empty()) {
				log_messages.printf(MSG_CRITICAL, "bad WU XML: %s\n",
						wu.xml_doc
				);
				errored=true;
			}
			if (!no_delete && f_write) {
				dir_hier_path(
					filename.c_str(), config.download_dir,
					config.uldl_dir_fanout, path, false
				);

				retval = unlink(path);

				if (retval && errno!=ENOENT) {
					log_messages.printf(MSG_CRITICAL,
							"[WU#%lu] unlink %s failed: %s\n",
							wu.id, filename.c_str(), boincerror(retval)
					);
					errored= true;
				}

				// delete the gzipped version of the file
				//
				sprintf(path_gz, "%s.gz", path);

				retval = unlink(path_gz);

				if (retval && errno!=ENOENT) {
					log_messages.printf(MSG_CRITICAL,
							"[WU#%lu] unlink %s.gz failed: %s\n",
							wu.id, filename.c_str(), boincerror(retval)
					);
					errored= true;
				}

				// delete the cached MD5 file if needed
				//
				if (config.cache_md5_info) {
					sprintf(path_md5, "%s.md5", path);

					retval = unlink(path_md5);

					if (retval && errno!=ENOENT) {
						log_messages.printf(MSG_CRITICAL,
								"[WU#%lu] unlink %s.md5 failed: %s\n",
								wu.id, filename.c_str(), boincerror(retval)
						);
						errored= true;
					}
				}
			}
		}
	}
	return !errored;
}


int delete_result_files (RESULT& result)
{
	char pathname[MAXPATHLEN];
	bool no_delete=false;
	int count_deleted = 0, retval, mthd_retval = 0;
	bool errored= false;

	MIOFILE mf;
	mf.init_buf_read(result.xml_doc_in);
	XML_PARSER xp(&mf);
	while (!xp.get_tag()) {
		if (!xp.is_tag) continue;
		if (xp.match_tag("file_info")) {
			string filename;
			no_delete = false;
			while (!xp.get_tag()) {
				if (xp.parse_string("name", filename)) {
						continue;
				} else if (xp.parse_bool("no_delete", no_delete)) {
						continue;
				} else if (xp.match_tag("/file_info")) {
						break;
				}
			}
			if (!xp.match_tag("/file_info") || filename.empty()) {
				log_messages.printf(MSG_CRITICAL, "bad result XML: %s\n",
						result.xml_doc_in
				);
				errored=true;
			}
			if (!no_delete && f_write) {
				dir_hier_path(
					filename.c_str(), config.upload_dir,
					config.uldl_dir_fanout, pathname, false
				);

				retval = unlink(pathname);

				if (retval && errno!=ENOENT) {
					log_messages.printf(MSG_CRITICAL,
							"[RESULT#%lu] unlink %s error: %s %s\n",
							result.id, pathname, boincerror(retval),
							(retval && errno)?strerror(errno):""
					);
					errored=true;
				}
			}
		}
	}

	return !errored;
}

unsigned count_deleted_w;
unsigned count_deleted_r;

void delete_old(const char* app_name, long cnt_limit, long delay, bool del_files, bool inp_blobs, bool out_blobs )
{
	cerr<<"delete_old("<<app_name<<") f_write="<<f_write<<" limit="<<cnt_limit<<" del_files="<<del_files<<" inp_blobs="<<inp_blobs<<" out_blobs="<<out_blobs<<endl;
	DB_APP app;
	{
		std::stringstream qr;
		qr<<"where name='"<<app_name<<"'";
		if (app.lookup(qr.str().c_str()))
			throw EDatabase("can't find app");
	}
	time_t cutoff_time;
	cutoff_time= time(0) - delay; //1209600
	std::stringstream qr;
	qr<<"where ";
	qr<<"appid="<<app.id<<" and ";
	qr<<"file_delete_state=1 and UNIX_TIMESTAMP(mod_time)<"<<cutoff_time<<" limit "<<cnt_limit<<";";
	DB_WORKUNIT wu;
	while(1) {
		int retval= wu.enumerate(qr.str().c_str());
		if(retval) {
			if (retval != ERR_DB_NOT_FOUND)
				throw EDatabase("enumerate workunits");
			break;
		}

		//del wu files
		bool all_results_deleted=true;
		bool wu_deleted= true;
		//del result files, if ok, del result
		DB_RESULT result;
		std::stringstream qr2;
		qr2<<"where workunitid="<<wu.id<<";";
		while(1) {
			retval= result.enumerate(qr2.str().c_str());
			if(retval) {
				if (retval != ERR_DB_NOT_FOUND)
					throw EDatabase("enumerate results");
				break;
			}
			bool result_deleted= true;
			if(del_files) {
				result_deleted &= delete_result_files(result);
			}
			if(out_blobs) {
				std::stringstream qr;
				qr << "delete from result_file where app="<<app.id<<" and res="<<result.id<<";";
				retval=boinc_db.do_query(qr.str().c_str());
				if( retval ) {
					log_messages.printf(MSG_CRITICAL,"[RESULT#%lu] result_file delete error\n", result.id);
					result_deleted = false;
				}
			}
			if (result_deleted) {
				if( retval=result.delete_from_db() ) {
					log_messages.printf(MSG_CRITICAL,
							"[RESULT#%lu] db delete error: %s\n",
							result.id, boincerror(retval)
					);
					cerr<<"R"<<result.id<<" db delete error"<<endl;
					all_results_deleted= false;
				} else
				count_deleted_r++;
			} else
				all_results_deleted= false;
		}
		if(all_results_deleted && del_files) {
			wu_deleted &= delete_workunit_files(wu);
		}
		if(all_results_deleted && inp_blobs) {
			std::stringstream qr;
			qr << "delete from input_file where wu="<<wu.id<<";";
			retval=boinc_db.do_query(qr.str().c_str());
			if( retval ) {
				log_messages.printf(MSG_CRITICAL,"[WU#%lu] input_file delete error\n", wu.id);
				wu_deleted= false;
			}
		}
		//if wu files deleted, delete wu
		if(all_results_deleted && wu_deleted) {
			if( retval=wu.delete_from_db() ) {
				log_messages.printf(MSG_CRITICAL,
						"[WU#%lu] db delete error: %s\n",
						wu.id, boincerror(retval)
				);
			} else
			count_deleted_w++;
		}
	}
}

int main(int argc, char** argv) {
	long gen_limit;
	int batchno;
	char *check1, *check2;
	count_deleted_w= count_deleted_r= 0;
	if(argc!=3) {
			cerr<<"Expect 2 command line argument: f_write limit"<<endl;
			exit(2);
	}
	f_write = (argv[1][0]=='y');
	gen_limit = strtol(argv[2],&check2,10);
	if((argv[1][0]!='n' && !f_write) || *check2) {
			cerr<<"Invalid argument format"<<endl;
			exit(2);
	}
	//connect db if requested
	initz();
	if(boinc_db.start_transaction())
		exit(4);

	delete_old("tot5", gen_limit, 1209600, 0, 1, 1);

	if(f_write) {
		if(boinc_db.commit_transaction()) {
			cerr<<"Can't commit transaction!"<<endl;
			exit(1);
		}
	}
	cerr<<"Deleted "<<count_deleted_r<<" results and "<<count_deleted_w<<" wus, of "<<gen_limit<<" max"<<endl;
	boinc_db.close();
	return 0;
}
