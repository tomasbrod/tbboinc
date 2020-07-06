#include <iostream>
#include <vector>
#include <map>
#include <list>

const int max_trans = 0x200000;

typedef std::vector<unsigned char> kvadrat;
typedef std::vector<unsigned char> transver;
typedef std::vector<char> kvadrat_m;
typedef std::map<kvadrat_m,std::list<kvadrat_m>>::iterator iter_mar;

extern int por;
extern int raz;
extern int raz_kvm;
extern std::vector<transver> baza_trans;

bool is_dlk(const kvadrat& kvad);

class exact_cover{
	static const int sdvig = 11;
	static const int modul = 0x7ff; // modul = 2^sdvig - 1

	struct data{
		data* left;
		data* right;
		data* up;
		data* down;
		union{
			unsigned column;
			unsigned size;
		};
	};

	static std::vector<data*> O;
	static std::vector<data> heads;
	static std::vector<data> nodes;

	static void save_solution(std::list<kvadrat_m>& templ){
		kvadrat mate(raz);
		for(int i = 0; i < por; i++){
			for(int j = 0, k = 0; j < por; k += por, j++){
				mate[k + baza_trans[O[i]->column >> sdvig][j]] = i;
			}
		}
		kvadrat_m tempk(raz_kvm);
		char* p = tempk.data();
		for(int i = 0; i < raz; i += por){
			*p++ = mate[i] + (mate[i] < 10 ? '0' : 'A' - 10);
			for(int j = 1; j < por; j++){
				*p++ = ' ';
				*p++ = mate[i + j] + (mate[i + j] < 10 ? '0' : 'A' - 10);
			}
			*p++ = '\r';
			*p++ = '\n';
		}
		templ.push_back(tempk);
	}

	static data* choose_column(){
		data* c = heads.data()->right;
		unsigned s = c->size;
		if(!s) return c;
		for(data* j = c->right; j != heads.data(); j = j->right) if(j->size < s){
			c = j;
			if(!(s = j->size)) return c;
		}
		return c;
	}

	static void cover_column(data* p){
		p->right->left = p->left;
		p->left->right = p->right;
		for(data* i = p->down; i != p; i = i->down){
			for(data* j = i->right; j != i; j = j->right){
				j->down->up = j->up;
				j->up->down = j->down;
				heads[j->column & modul].size--;
			}
		}
	}

	static void uncover_column(data* p){
		for(data* i = p->up; i != p; i = i ->up){
			for(data* j = i->left; j != i; j = j->left){
				heads[j->column & modul].size++;
				j->down->up = j->up->down = j;
			}
		}
		p->right->left = p->left->right = p;
	}

	static void init(int ch_trans);
	static void search(std::list<kvadrat_m>& templ);
	static void init_trans(const kvadrat& dlk);
	static void search_trans(const kvadrat& dlk);

public:
	exact_cover(){
		O.resize(por);
		heads.resize(raz + 1);
	}

	static void search_mates(const kvadrat& dlk);
};