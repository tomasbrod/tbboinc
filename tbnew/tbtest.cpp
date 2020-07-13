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


template <class cfg>
struct DLX : public cfg {
	using cfg::nheads;
	using cfg::node;
	using cfg::path;
	using cfg::lvl;
	using typename cfg::Index;
	using typename cfg::Node;
	void init_heads () {
		lvl = 0;
		// initialize n+1 column heads, the last one is a header for the heads
		for(int i = 0; i <= nheads; i++){
			node[i].right   = node + i + 1;
			node[i].left  = node + i - 1;
			node[i].down = node + i;
			node[i].up   = node + i;
			node[i].count = 0;
		}
		node[0].left = node + nheads; // first points to last
		node[nheads].right = node + 0; // last points to first
	}
	void link_node_init(Index i, Index t) {
		Node& n= node[i];
		n.right = node + i+1;
		n.left = node + i-1;
		n.top = t; n.down = node + t;
		n.up = node[t].up;
		node[t].up = node[t].up->down = node + i;
		node[t].count ++;
	}
	Node* choose_column() {
		Node* c = node[nheads].right;
		int s = c->count;
		for (Node* j = c; j!=node+nheads; j=j->right) {
			if( j->count < s ) {
				s = j->count;
				if(!s) return j;
				c = j;
			}
		}
		return c;
	}
	void link_error() {
		cout<<"Link Error!"<<endl;
		exit(1);
	}
	void check_heads() {
		for(Index h=node[nheads].right; h!=nheads; h= node[h].right) {
			cout<<" h"<<h;
		}
		cout<<" - ";
		for(Index h=node[nheads].left; h!=nheads; h= node[h].left) {
			cout<<" h"<<h;
		}
		cout<<endl;
	}
	void cover(Node* n) {
		Node* j = n;
		do {
			Node* c = node+j->top;
			//cout<<" unlink head "<<c << " l"<<node[c].left<<" r"<<node[c].right<<endl;
			//if((node[node[c].left].right!=c)||(node[node[c].right].left!=c)) link_error();
			c->left->right = c->right;
			c->right->left = c->left;
			for(Node* r = c->down; r!=c; r = r->down) {
				for(Node* k = r->right; k!=r; k = k->right) {
					//cout<<" unlink node "<<k<<" u"<<node[k].up<<" d"<<node[k].down<<endl;
					//if((node[node[k].up].down!=k)||(node[node[k].down].up!=k)) link_error();
					k->up->down = k->down;
					k->down->up = k->up;
					node[k->top].count -- ;
				}
			}
			j = j->right;
		} while(j!=n);
	}
	void uncover(Node* n) {
		Node* j = n;
		do {
			j = j->left;
			Node* c = node + j->top;
			for(Node* r = c->up; r!=c; r = r->up) {
				for(Node* k = r->left; k!=r; k = k->left) {
					//cout<<" link node "<<k<<" u"<<node[k].up<<" d"<<node[k].down<<endl;
					k->down->up = k;
					k->up->down = k;
					node[k->top].count ++ ;
				}
			}
			//cout<<" link head "<<c << " l"<<node[c].left<<" r"<<node[c].right<<endl;
			c->right->left = c;
			c->left->right = c;
		} while(j!=n);
	}

	bool  __attribute__ ((noinline)) search () {
			Node* c; // column
			Node* r; // row
			//cout<<"begin search"<<endl;

			if (lvl) goto backtrack; // to find next solution backtrack

		advance:
			c = choose_column();
			//cout<<"next column "<<c<<endl;
			if( c == node+nheads ) return true; // no more columns -> solved
			r = c->down;	// select first available row

		descend:
			if( r == c ) goto backtrack;
			//cout<<"descend "<<lvl<<" into "<<r<<endl;
			path[lvl++] = r; // add row to path
			cover(r);
			goto advance;

		backtrack:
			if( !lvl ) return false; // cant backtrack -> no more solutions
			r = path[--lvl]; // remove from path
			//cout<<"backtrack "<<lvl<<" into "<<r<<endl;
			c= node + r->top;
			uncover(r);
			r = r->down; // select next available row
			goto descend;
	}
};

struct trans_dlx_cfg {
	/* Configuration for DLX to find transversals of latin square of order 10,
	 * and to find disjoint set of transversals.
	 * There are 32+1  heads and 320 nodes for transversal enumeration.
	 * There are 100+1 heads and xxx nodes for disjoint set search.
	 * Therefore uint16 is chosen for index type.
	 * */
	typedef uint16_t Index;
	struct Node {
		/* pointers needed for both heads and nodes */
		Node* down;
		Node* up;
		/* also needed for both, but could be optimized away as described in Knuth's 5c */
		Node* right;
		Node* left;
		union {
			struct { // head alternative
				uint16_t count; // count of nodes in this column
			};
			struct { // node alternative
				/* TOP max value = 100 -> 7 bits */
				unsigned top : 7;
				/* row/col from the LK, (derive from address?) */
				unsigned row : 4;
				unsigned col : 4;
			};
		};
	};
	static const int max_trans =  6544; // absolute max for uint16_t is ceil((65536-101)/10)
	unsigned nheads; // number of heads in current problem (32 or 100)
	Node node[max_trans*10 + 101];
	std::array<Node*, 10 > path; // solution path
	Index lvl; // current level (0 on init, 10 on solution)
};

static DLX<trans_dlx_cfg> dlx{};

void init_trans(const kvadrat& lk) {
	dlx.nheads = 32;
	dlx.init_heads();
	auto& node = dlx.node;
	unsigned count;
	count = 33;
	unsigned short temp[5];
	unsigned t;
	for(int i = 0; i < raz; i++){
		temp[0] = i / 10;
		temp[1] = i % 10;
		temp[2] = lk[i];
		temp[3] = temp[0] == temp[1] ? 0 : temp[0] == 9 - temp[1] ? 1 : 10;
		temp[4] = 10;
		for(int j = 0; temp[j] < 10; j++) {
			t = temp[j] + j*10;
			//cout <<i<<":"<<j<<" n"<<(count+j)<<" to "<<t<<endl;
			auto& n = node[count+j];
			n.row=temp[0];
			n.col=temp[1];
			dlx.link_node_init(count+j,t);
		}
		//finish_row_init(count,)
		if(temp[3]<10){
			node[count+3].right = node+count; // loop back forwards
			node[count].left += 4; // loop front backwards
			count += 4;
		}
		else{
			node[count+2].right = node+count;
			node[count].left += 3;
			count += 3; // use 3 here for better packing. 4 is for debugging
		}
	}
	cout<<"node count "<<count<<endl;
	#if 0
	cout<<"checking heads"<<endl;
	for(int h=0; h<33; h++) {
		if(node[h].left!=((h+32)%33))
			cout<<"err at "<<h<<" l"<<node[h].left<<endl;
		if(node[h].right!=((h+1)%33))
			cout<<"err at "<<h<<" r"<<node[h].right<<endl;
		for(int n=node[h].down; n!=h; n=node[n].down) {
			if(node[n].top!=h)
				cout<<"err at "<<n<<" h"<<node[n].top<<endl;
			if( ((n-33)%4) != (h/10) )
				cout<<"err at "<<n<<" not in right column "<<h<<endl;
		}
	}
	cout<<"checking rows"<<endl;
	for(int r=0; r<100; ++r) {
		int ni= 33 + r*4;
		int rcnt=0;
		int n = ni;
		do {
			if(n!=ni&&node[n].left!=n-1)
				cout<<"err at "<<n<<" l"<<node[n].left<<endl;
			if( ((n-33)/4)!=r )
				cout<<"err at "<<n<<" not in right row"<<r<<endl;
			n=node[n].right;
			rcnt++;
			if(rcnt>4)
				cout<<"err at r"<<r<<" too many nodes in row "<<rcnt<<endl;
		} while(n!=ni);
		if(node[ni].left!=ni+rcnt-1)
			cout<<"err at "<<ni<<" l"<<node[ni].left<<" c"<<rcnt<<endl;
		if(rcnt<3)
			cout<<"err at r"<<r<<" too little nodes in row "<<rcnt<<endl;
				
	}
	#endif
}

typedef std::array<unsigned char, por> transver;


void init_orto (const transver baza_trans[], const int cnt_trans)
{
	dlx.nheads = 100;
	dlx.init_heads();
	auto& node = dlx.node;
	unsigned count;
	count = 101;
	unsigned t;
	for(int i = 0; i < cnt_trans; i++){
		for(int j = 0, k = 0; j<10; k+=10, j++) {
			t = baza_trans[i][j] + k;
			//cout <<i<<":"<<j<<" n"<<(count+j)<<" to "<<t<<endl;
			dlx.link_node_init(count+j,t);
		}
		node[count+9].right = node+count;
		node[count].left += 10;
		count += 10;
	}
	cout<<"node count "<<count<<endl;
}

const kvadrat testlk = {
0, 2, 3, 7, 8, 9, 5, 6, 4, 1,
3, 1, 8, 5, 0, 7, 9, 4, 2, 6,
8, 5, 2, 6, 3, 1, 7, 0, 9, 4,
2, 7, 1, 3, 9, 8, 4, 5, 6, 0,
1, 8, 0, 9, 4, 6, 3, 2, 5, 7,
6, 9, 4, 0, 7, 5, 2, 8, 1, 3,
7, 0, 5, 8, 1, 4, 6, 9, 3, 2,
4, 6, 9, 2, 5, 3, 1, 7, 0, 8,
9, 3, 7, 4, 6, 2, 0, 1, 8, 5,
5, 4, 6, 1, 2, 0, 8, 3, 7, 9
};

int main(void) {
	cout<<"node size "<<sizeof(trans_dlx_cfg::Node)<<endl;
	clock_t t0= clock();
	init_trans (testlk);
	transver baza_trans[trans_dlx_cfg::max_trans];
	int cnt_trans = 0;
	int c = 0;
	while (bool r = dlx.search()) {
		transver& tr = baza_trans[cnt_trans++];
		for(int i = 0; i < por; i++){
			auto& n = *dlx.path[i];
			tr[n.row] = n.col;
		}
	}
	cout << "Run Time (s): " << double(clock() - t0) / CLOCKS_PER_SEC << endl;
	cout << "transversals: "<< cnt_trans <<endl;
	t0= clock();
	init_orto (baza_trans, cnt_trans);
	while (bool r = dlx.search()) {
		c+=1;
	}
	cout << "Run Time (s): " << double(clock() - t0) / CLOCKS_PER_SEC << endl;
	cout<<"solutions: "<<c<<endl;
	return 69;
	
	/* a) one thread searches transversals and another checks disjoint
	 * b) search transversals, disjoint, repeat
	 * dlx.find_trans(lk);
	 * //dlx.ntrans, dlx.trans
	 * dlx.init_disjoint();
	 * dlx.find_mate();
	 * dlx.get_mate();
	*/
}
/* the question is: does natalia's generator produce isomorfizms? */
