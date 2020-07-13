#include <iostream>
#include <array>
#include <map>
#include <list>
#include <iostream>
#include <fstream>
#include <list>
#include <set>
#include <ctime>
#include <unistd.h>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <thread>
#include <mutex>
#include "odlkcommon/common.h"
using namespace std;

struct SerSNDLK {
	kvadrat& lk;
	SerSNDLK(kvadrat& ilk) :lk(ilk) {};
	//...
}

struct InitData {
	int rule;
	kvadrat start;
	int level;
	unsigned long upload;
	bool skip;
	bool skip_less;
	array<unsigned, 3> skip_rule;

	void Serialize(MemStream& s) {
		s.w1(rule).w1(level).w1(skip_less).w4(upload_period);
		for(int i=0; i<3; ++i) s.w4(skip_rule[i]);
		for(int i=0; i<100; i+=2) s.w1(start[i]<<8 | start[i+1]);
	}
	void Deser(MemStream& s) {
		s.r1(rule).r1(level).r1(skip_less).r4(upload_period);
		for(int i=0; i<3; ++i) s.r4(skip_rule[i]);
		for(int i=0, v; i<100; i+=2) { s.r1(v); start[i]=v>>8; start[i+1]=v&15; }
		skip = skip_less || !skip_rule.empty();
	}
};

struct State {
	InitData init;
	kvadrat last;
	list<Sndlk> mar;
	unsigned max_trans, max_dtrans;
	Sndlk max_trans_lk, max_dtrans_lk;
	unsigned long ncf;
	unsigned long long nsn;
	unsigned long long ntrans;
	unsigned long long ndaugh;

	void Serialize(MemStream& s) {
		init.Serialize(s);
		s .w4(mar.size()) .w4(max_trans) .w4(max_dtrans) .w4(ncf);
		s .w8(nsn) .w8(ntrans) .w8(ndaugh)
		for(int i=0; i<100; i+=2) s.w1(last[i]<<8 | last[i+1]);

		for(int i=0; i<3; ++i) s.w4(skip_rule[i]);
		for(int i=0; i<100; i+=2) s.w1(start[i]<<8 | start[i+1]);
	}
	void Deser(MemStream& s) {
		s.r1(rule).r1(level).r1(skip_less).r4(upload_period);
		for(int i=0; i<3; ++i) s.r4(skip_rule[i]);
		for(int i=0, v; i<100; i+=2) { s.r1(v); start[i]=v>>8; start[i+1]=v&15; }
		skip = skip_less || !skip_rule.empty();
	}
} state;

int main(void) {
	initialize
	resume
}
