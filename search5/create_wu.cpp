
class DB_SEGMENT : public DB_BASE {
public:
	//needed: rule, minl, next
	DB_ID_TYPE id;
	int rule;
	//ix
	//start
	int minl;
	NamerCHDLK10::NameStr next;
	bool enabled;
	//cur_wu
	int prio_adjust; //not where it should be
public:
    DB_SEGMENT(DB_CONN* p=0) : DB_BASE("tot_segment", p?p:&boinc_db) {}
    void db_parse(MYSQL_ROW &r) {
			id = atoi(r[0]);
			rule = atoi(r[1]);
			minl = atoi(r[4]);
			std::copy(r[5],r[5]+next.size(),next.begin());
			enabled = atoi(r[6]);
			prio_adjust=0;
		}
};

struct gen_padls_cfg {
	bool write;
	int batch;
	DB_APP app;
	std::string in_template;
};


int create_work3(
    DB_WORKUNIT& wu,
    const char* result_template_filename,
        // relative to project root; stored in DB
    SCHED_CONFIG& config_loc,
    const CStream& input_data
) {
    int retval;

    wu.create_time = time(0);

		unsigned long in_len = input_data.pos();
		char in_md5[256];
		md5_block((const unsigned char*)input_data.getbase(), in_len, in_md5);
    snprintf(wu.xml_doc, sizeof(wu.xml_doc),
    "<file_info>\n<name>%s.in</name>\n"
    "<url>https://boinc.tbrada.eu/tbrada_cgi/fuh?%s.in</url>\n"
    "<md5_cksum>%s</md5_cksum>\n<nbytes>%lu</nbytes>\n</file_info>\n"
    "<workunit>\n<file_ref>\n<file_name>%s.in</file_name>\n"
    "<open_name>input.dat</open_name>\n</file_ref>\n</workunit>\n"
    , wu.name, wu.name
    , in_md5, in_len
    , wu.name
    );

    // check for presence of result template.
    // we don't need to actually look at it.
    //
    const char* p = config_loc.project_path(result_template_filename);
    if (!boinc_file_exists(p)) {
        fprintf(stderr,
            "create_work: result template file %s doesn't exist\n", p
        );
        return retval;
    }

    if (strlen(result_template_filename) > sizeof(wu.result_template_file)-1) {
        fprintf(stderr,
            "result template filename is too big: %d bytes, max is %d\n",
            (int)strlen(result_template_filename),
            (int)sizeof(wu.result_template_file)-1
        );
        return ERR_BUFFER_OVERFLOW;
    }
    strncpy(wu.result_template_file, result_template_filename, sizeof(wu.result_template_file));

    if (wu.rsc_fpops_est == 0) {
        fprintf(stderr, "no rsc_fpops_est given; can't create job\n");
        return ERR_NO_OPTION;
    }
    if (wu.rsc_fpops_bound == 0) {
        fprintf(stderr, "no rsc_fpops_bound given; can't create job\n");
        return ERR_NO_OPTION;
    }
    if (wu.rsc_disk_bound == 0) {
        fprintf(stderr, "no rsc_disk_bound given; can't create job\n");
        return ERR_NO_OPTION;
    }
    if (wu.target_nresults == 0) {
        fprintf(stderr, "no target_nresults given; can't create job\n");
        return ERR_NO_OPTION;
    }
    if (wu.max_error_results == 0) {
        fprintf(stderr, "no max_error_results given; can't create job\n");
        return ERR_NO_OPTION;
    }
    if (wu.max_total_results == 0) {
        fprintf(stderr, "no max_total_results given; can't create job\n");
        return ERR_NO_OPTION;
    }
    if (wu.max_success_results == 0) {
        fprintf(stderr, "no max_success_results given; can't create job\n");
        return ERR_NO_OPTION;
    }
    if (wu.max_success_results > wu.max_total_results) {
        fprintf(stderr, "max_success_results > max_total_results; can't create job\n");
        return ERR_INVALID_PARAM;
    }
    if (wu.max_error_results > wu.max_total_results) {
        fprintf(stderr, "max_error_results > max_total_results; can't create job\n");
        return ERR_INVALID_PARAM;
    }
    if (wu.target_nresults > wu.max_success_results) {
        fprintf(stderr, "target_nresults > max_success_results; can't create job\n");
        return ERR_INVALID_PARAM;
    }
    /*
    auto prev_transitioner_flags= wu.transitioner_flags;
    wu.transitioner_flags= 1;
    */
    if (wu.transitioner_flags) {
        wu.transition_time = INT_MAX;
    } else {
        wu.transition_time = time(0);
    }

		retval = wu.insert();
		if (retval) {
				fprintf(stderr,
						"create_work: workunit.insert() %s\n", boincerror(retval)
				);
				return retval;
		}
		wu.id = boinc_db.insert_id();

		//insert input_file
		MYSQL_STMT* insert_stmt = 0;
		insert_stmt = mysql_stmt_init(boinc_db.mysql);
		char stmt[] = "insert into input_file SET wu=?, data=?";
		void* in_data= (void*)input_data.getbase();
		MYSQL_BIND bind[] = {
				{.buffer=&wu.id, .buffer_type=MYSQL_TYPE_LONG, 0},
				{.length=&in_len, .buffer=in_data, .buffer_type=MYSQL_TYPE_BLOB, 0},
		};
		if(!insert_stmt
				|| mysql_stmt_prepare(insert_stmt, stmt, sizeof stmt )
				|| mysql_stmt_bind_param(insert_stmt, bind)
				|| mysql_stmt_execute(insert_stmt)
		) {
				mysql_stmt_close(insert_stmt);
				fprintf(stderr,
						"create_work: insert of input_data failed %s\n", mysql_error(boinc_db.mysql) );
				//wu.delete_from_db();
				return -1;
		}
		mysql_stmt_close(insert_stmt);

		/*
		wu.transitioner_flags= prev_transitioner_flags;
    if (wu.transitioner_flags) {
        wu.transition_time = INT_MAX;
    } else {
        wu.transition_time = time(0);
    }
		wu.update();
		*/

    return 0;
}

void gen_padls_wu(DB_SEGMENT& item, gen_padls_cfg& cfg) {
	// TODO: if enabled
	if(!item.enabled)
		return;
	Input inp;
	CDynamicStream buf;
	inp.rule= item.rule;
	if(!NamerCHDLK10::fromName58(item.next,inp.start))
		throw EDatabase("Bad sndlk in segment row");
	inp.min_level= item.minl;
	inp.skip_below= 0;
	inp.skip_fast= 0;
	inp.skip_rule= {0};
	inp.lim_sn= 3815000000;
	inp.lim_kf= 224000;
	std::stringstream wuname;
	((wuname<<"tot5_"<<item.rule<<char(cfg.batch+'a'))<<"_").write(item.next.data(),item.next.size());
	cout<<" WU "<<wuname.str()<<endl;
	if(!cfg.write)
		return;
	inp.writeInput(buf);

	DB_WORKUNIT wu; wu.clear();
	wu.appid = cfg.app.id;
	wu.batch=20+cfg.batch;
	strcpy(wu.name, wuname.str().c_str());
	wu.rsc_fpops_est = 4e13; // 2x 1h
	wu.rsc_fpops_bound = 1e16;
	wu.rsc_memory_bound = 1e8; //todo 100M
	wu.rsc_disk_bound = 1e8; //todo 100m
	wu.delay_bound = 604800; // 7 days
	wu.priority = 1 + item.prio_adjust;
	wu.target_nresults= wu.min_quorum = 1;
	wu.max_error_results= wu.max_total_results= 8;
	wu.max_success_results= 1;

	retval= create_work3(wu, "templates/tot5_out",config, buf);

	if(retval) throw EDatabase("create_work3 failed");
	std::stringstream qr{};
	qr<<"update tot_segment set cur_wu="<<wu.id<<" where id="<<item.id<<" limit 1;";
	retval = boinc_db.do_query(qr.str().c_str());
	if(retval || boinc_db.affected_rows()!=1) {
		throw EDatabase("segment update error");
	}
}
