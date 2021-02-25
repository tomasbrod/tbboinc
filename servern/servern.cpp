#include <stdio.h>
#include <string>
#include "boinclib/miofile.h"
#include <fcgiapp.h>
using std::string;
#include "parse2.cpp"

int main(void) {

	FILE* f = fopen("foo2.xml", "r");
	if (!f) {
		fprintf(stderr, "no file\n");
		exit(1);
	}
	parse_test(f);
	fclose(f);

	FILE* f2 = fopen("sched_request_boinc.tbrada.eu.xml", "r");
	if (!f2) {
		fprintf(stderr, "no file\n");
		exit(1);
	}
	parse_test_3(f2);
	fclose(f2);

	// run scheduler in stdio mode
	MIOFILE clientin, clientout;
	clientin.init_file(stdin);
	clientout.init_file(stdout);

	return 1;
}
