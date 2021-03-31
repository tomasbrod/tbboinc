#include <stdio.h>
#include <string>
#include "parse2.hpp"
#include "boinclib/miofile.h"
#include <fcgiapp.h>
#include "build/cofig.hpp"
#include "build/cofig.cpp"
#include "build/defs.hpp"
#include "build/defs.cpp"
using std::string;

void parse_test(FILE* f);
void parse_test_3(FILE* f);
void kv_test();

int main(void) {

	FILE* f2 = fopen("sched_request_boinc.tbrada.eu.xml", "r");
	if (!f2) {
		fprintf(stderr, "no file\n");
		exit(1);
	}
	parse_test_3(f2);
	fclose(f2);

	FILE* f1 = fopen("foo2.xml", "r");
	if (!f1) {
		fprintf(stderr, "no file\n");
		exit(1);
	}
	parse_test(f1);
	fclose(f1);

	FILE* f3 = fopen("config.xml", "r");
	if (!f2) {
		fprintf(stderr, "no file\n");
		exit(1);
	}
	{
		CFileStream fs3 (f3);
		XML_PARSER2 xp (&fs3);
		t_config config;
		xp.get_tag(); // open the root tag, todo check error code
		config.parse(xp);
	}
	fclose(f2);

	//kv_test();

	// run scheduler in stdio mode
	MIOFILE clientin, clientout;
	clientin.init_file(stdin);
	clientout.init_file(stdout);

	return 1;
}
