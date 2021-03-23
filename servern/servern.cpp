#include <stdio.h>
#include <string>
#include "boinclib/miofile.h"
#include <fcgiapp.h>
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

	//kv_test();

	// run scheduler in stdio mode
	MIOFILE clientin, clientout;
	clientin.init_file(stdin);
	clientout.init_file(stdout);

	return 1;
}
