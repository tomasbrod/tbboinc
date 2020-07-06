#include "exact_cover_u.h"

	std::vector<transver> baza_trans;
	std::vector<exact_cover::data*> exact_cover::O;
	std::vector<exact_cover::data> exact_cover::heads;
	std::vector<exact_cover::data> exact_cover::nodes;

bool is_dlk(const kvadrat& kvad){
	long long flag, t;
	for(int i = 0; i < raz; i += por){
		flag = 0;
		for(int j = 0; j < por; j++){
			if(flag & (t = 1ll << kvad[i + j])) return false;
			flag |= t;
		}
	}
	for(int i = 0; i < por; i++){
		flag = 0;
		for(int j = 0; j < raz; j += por){
			if(flag & (t = 1ll << kvad[i + j])) return false;
			flag |= t;
		}
	}
	flag = 0;
	for(int j = 0; j < raz + por; j += por + 1){
		if(flag & (t = 1ll << kvad[j])) return false;
		flag |= t;
	}
	flag = 0;
	for(int j = por - 1; j <= raz - por; j += por - 1){
		if(flag & (t = 1ll << kvad[j])) return false;
		flag |= t;
	}
	return true;
}

void exact_cover::search(std::list<kvadrat_m>& templ){
	data* c;
	data* r;
	int l = 0;
forward:
	c = choose_column();
	if(!c->size) goto backtrack;
	cover_column(c);
	r = O[l] = c->down;
advance:
	if(r == c) goto backup;
	if(l == por - 1){
		save_solution(templ);
		goto recover;
	}
	for(data* j = r->right; j != r; j = j->right) cover_column(heads.data() + (j->column & modul));
	l++;
	goto forward;
backup:
	uncover_column(c);
backtrack:
	if(l == 0) return;
	r = O[--l];
	c = heads.data() + (r->column & modul);
	for(data* j = r->left; j != r; j = j->left) uncover_column(heads.data() + (j->column & modul));
recover:
	r = O[l] = r->down;
	goto advance;
}

void exact_cover::init(int ch_trans){
	nodes.resize(ch_trans * por);
	for(int i = 0; i <= raz; i++){
		heads[i].right = heads.data() + i + 1;
		heads[i].left = heads.data() + i - 1;
		heads[i].down = heads[i].up = heads.data() + i;
		heads[i].size = 0;
	}
	heads[0].left += raz + 1;
	heads[raz].right = heads.data();
	for(int i = 0, count = 0; i < ch_trans; count += por, i++){
		for(int j = 0, k = 0, t; j < por; k += por, j++){
			nodes[count + j].column = (i << sdvig) | (t = baza_trans[i][j] + k + 1);
			nodes[count + j].right = nodes.data() + count + j + 1;
			nodes[count + j].left = nodes.data() + count + j - 1;
			nodes[count + j].down = heads.data() + t;
			nodes[count + j].up = heads[t].up;
			heads[t].up = heads[t].up->down = nodes.data() + count + j;
			heads[t].size++;
		}
		nodes[count].left += por;
		nodes[count + por - 1].right -= por;
	}
}

void exact_cover::search_mates(const kvadrat& dlk){
	baza_trans.clear();
	search_trans(dlk);
	int ch_trans = baza_trans.size();
	if(ch_trans < por) return;
	init(ch_trans);
	std::list<kvadrat_m> templ;
	search(templ);
	if(templ.empty()) return;
	kvadrat_m tempk(raz_kvm);
	char* p = tempk.data();
	for(int i = 0; i < raz; i += por){
		*p++ = dlk[i] + (dlk[i] < 10 ? '0' : 'A' - 10);
		for(int j = 1; j < por; j++){
			*p++ = ' ';
			*p++ = dlk[i + j] + (dlk[i + j] < 10 ? '0' : 'A' - 10);
		}
		*p++ = '\r';
		*p++ = '\n';
	}
	extern std::map<kvadrat_m,std::list<kvadrat_m>> baza_mar;
	baza_mar.insert(std::make_pair(tempk, templ));
}

void exact_cover::search_trans(const kvadrat& dlk){
	init_trans(dlk);
	transver tempt(por);
	data* c;
	data* r;
	int l = 0, count = 0;
forward:
	c = choose_column();
	if(!c->size) goto backtrack;
	cover_column(c);
	r = O[l] = c->down;
advance:
	if(r == c) goto backup;
	if(l == por - 1){
		for(int i = 0, t; i < por; i++){
			t = O[i]->column >> sdvig;
			tempt[t >> 8] = t & 0xff;
		}
		baza_trans.push_back(tempt);
		if(++count > max_trans){
			std::cerr << "Число диагональных трансверсалей ДЛК" << por << ":\n\n";
			for(int i = 0; i < raz; i++) std::cerr << char(dlk[i] + (dlk[i] < 10 ? '0' : 'A' - 10)) << (i % por != por - 1 ? ' ' : '\n');
			std::cerr << "\nпревышает максимум " << max_trans << std::endl << std::endl;
			extern bool mute;
			if(!mute){
				std::cout << "Для выхода нажмите любую клавишу: ";
				system("pause > nul");
				std::cout << std::endl;
			}
			exit(3);
		}
		goto recover;
	}
	for(data* j = r->right; j != r; j = j->right) cover_column(heads.data() + (j->column & modul));
	l++;
	goto forward;
backup:
	uncover_column(c);
backtrack:
	if(l == 0) return;
	r = O[--l];
	c = heads.data() + (r->column & modul);
	for(data* j = r->left; j != r; j = j->left) uncover_column(heads.data() + (j->column & modul));
recover:
	r = O[l] = r->down;
	goto advance;
}

void exact_cover::init_trans(const kvadrat& dlk){
	nodes.resize(raz * 3 + (por << 1));
	int dl = 3 * por + 2;
	for(int i = 0; i <= dl; i++){
		heads[i].right = heads.data() + i + 1;
		heads[i].left = heads.data() + i - 1;
		heads[i].down = heads[i].up = heads.data() + i;
		heads[i].size = 0;
	}
	heads[0].left = heads.data() + dl;
	heads[dl].right = heads.data();
	int temp[4];
	for(int i = 0, count = 0, r, c; i < raz; i++){
		temp[0] = (r = i / por) + 1;
		temp[1] = (c = i % por) + por + 1;
		temp[2] = dlk[i] + (por << 1) + 1;
		temp[3] = 0;
		if(r == c) temp[3] = 1;
		if(r == por - 1 - c) temp[3] |= 2;
		int t;
		for(int j = 0, t; j < 3; j++){
			nodes[count + j].column = (r << (sdvig + 8)) | (c << sdvig) | (t = temp[j]);
			nodes[count + j].right = nodes.data() + count + j + 1;
			nodes[count + j].left = nodes.data() + count + j - 1;
			nodes[count + j].down = heads.data() + t;
			nodes[count + j].up = heads[t].up;
			heads[t].up = heads[t].up->down = nodes.data() + count + j;
			heads[t].size++;
		}
		switch(temp[3]){
			case 0:
				nodes[count].left += 3;
				nodes[count + 2].right -= 3;
				count += 3;
				break;
			case 1:
			case 2:
				nodes[count + 3].column = (r << (sdvig + 8)) | (c << sdvig) | (t = dl - 2 + temp[3]);
				nodes[count + 3].right = nodes.data() + count;
				nodes[count + 3].left = nodes.data() + count + 2;
				nodes[count + 3].down = heads.data() + t;
				nodes[count + 3].up = heads[t].up;
				heads[t].up = heads[t].up->down = nodes.data() + count + 3;
				heads[t].size++;
				nodes[count].left += 4;
				count += 4;
				break;
			case 3:
				for(int j = 0; j < 2; j++){
					nodes[count + 3 + j].column = (r << (sdvig + 8)) | (c << sdvig) | (t = dl - 1 + j);
					nodes[count + 3 + j].right = nodes.data() + count + j + 4;
					nodes[count + 3 + j].left = nodes.data() + count + j + 2;
					nodes[count + 3 + j].down = heads.data() + t;
					nodes[count + 3 + j].up = heads[t].up;
					heads[t].up = heads[t].up->down = nodes.data() + count + 3 + j;
					heads[t].size++;
				}
				nodes[count].left += 5;
				nodes[count + 4].right -= 5;
				count += 5;
		}
	}
}
