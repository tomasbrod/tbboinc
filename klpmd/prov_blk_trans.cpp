#include "kanonizator.h"

	trans_dlx::data* trans_dlx::O[];
	trans_dlx::data trans_dlx::heads[];
	vector<trans_dlx::data> trans_dlx::nodes;

void trans_dlx::search_trans(const kvadrat& lk){
	for(int i = 0; i < ch_srez; i++) trans[i].clear();
	init_trans(lk);
	transver tempt[ch_srez];
	data* c;
	data* r;
	int l = 0;
forward:
	if(!(c = choose_column())) goto backtrack;
	cover_column(c);
	r = O[l] = c->down;
advance:
	if(r == c) goto backup;
	if(l == por - 1){
		for(int i = 0, t; i < por; i++){
			t = O[i]->column >> 7;
			tempt[0][t >> 8] = (t >> 4) & 0xf;	// <r,c,v>
			tempt[1][t >> 8] = t & 0xf;			// <r,v,c>
			tempt[2][t & 0xf] = (t >> 4) & 0xf;	// <v,c,r>
		}
		for(int i = 0; i < ch_srez; i++) trans[i].push_back(tempt[i]);
		goto recover;
	}
	for(data* j = r->right; j != r; j = j->right) cover_column(heads + (j->column & 0x7f));
	l++;
	goto forward;
backup:
	uncover_column(c);
backtrack:
	if(l == 0) return;
	r = O[--l];
	c = heads + (r->column & 0x7f);
	for(data* j = r->left; j != r; j = j->left) uncover_column(heads + (j->column & 0x7f));
recover:
	r = O[l] = r->down;
	goto advance;
}

void trans_dlx::init_trans(const kvadrat& lk){
	for(int i = 0; i < ch_cols; i++){
		heads[i].right = heads + i + 1;
		heads[i].left = heads + i - 1;
		heads[i].down = heads[i].up = heads + i;
		heads[i].size = 0;
	}
	heads[0].left += ch_cols;
	heads[ch_cols - 1].right -= ch_cols;
	int temp[3];
	for(int i = 0, count = 0, r, c, v; i < raz; i++){
		temp[0] = (r = i / por) + 1;
		temp[1] = (c = i % por) + por + 1;
		temp[2] = (v = lk[i]) + (por << 1) + 1;
		for(int j = 0, t; j < 3; j++){
			nodes[count + j].column = (r << 15) | (c << 11) | (v << 7) | (t = temp[j]);
			nodes[count + j].right = nodes.data() + count + j + 1;
			nodes[count + j].left = nodes.data() + count + j - 1;
			nodes[count + j].down = heads + t;
			nodes[count + j].up = heads[t].up;
			heads[t].up = heads[t].up->down = nodes.data() + count + j;
			heads[t].size++;
		}
		nodes[count].left += 3;
		nodes[count + 2].right -= 3;
		count += 3;
	}
}

void trans_dlx::poisk_simm(const kvadrat& lk, const vector<transver>& tr, int srez){
	int rows[por], cols[por];
	bool flag = srez != 0;
	kvadrat dlk, kf;
	for(int i = 0, otr1[por]; i < cnt_trans - 1; i++){
		for(int k = 0; k < por; k++) otr1[tr[i][k]] = k;
		for(int j = i + 1, otr2[por]; j < cnt_trans; j++){
			for(int k = 0; k < por; k++) if(tr[i][k] == tr[j][k]) goto next;
			for(int k = 0; k < por; k++) otr2[tr[j][k]] = k;
			for(int k = 0, l = 0; k < por && l < n; k++)
				if(otr1[tr[j][k]] != otr2[tr[i][k]]) goto next;
				else if(k < otr2[tr[i][k]]){
					rows[l] = k;
					rows[por - 1 - l] = otr2[tr[i][k]];
					cols[l] = tr[i][k];
					cols[por - 1 - l] = tr[j][k];
					l++;
				}
			for(int ii = 0; ii < por; ii++) for(int jj = 0; jj < por; jj++) dlk[ii * por + jj] = lk[rows[ii] * por + cols[jj]];
			kanonizator::kanon(dlk, kf);
			if(flag){
				flag = false;
				if(!baza_kf.insert(kf).second) return;
				kf_trans[srez].push_back(make_pair(kf, make_pair(tr[i], tr[j])));
			}
			else if(baza_kf.insert(kf).second) kf_trans[srez].push_back(make_pair(kf, make_pair(tr[i], tr[j])));
			next:;
		}
	}
}

void trans_dlx::poisk_simm_dlx(const kvadrat& lk, const vector<transver>& tr, int srez){
	bool flag = srez != 0;
	transver tempt;
	kvadrat dlk, kf;
	data* c;
	data* r;
	for(int i = 0; i < ch_cols_simm; i++){
		heads[i].right = heads + i + 1;
		heads[i].left = heads + i - 1;
	}
	heads[0].left += ch_cols_simm;
	heads[ch_cols_simm - 1].right = heads;
	for(int i = 0, l; i < cnt_trans; i++){
		if(tr[i][0] == por - 1) continue;
		if(!init_simm(lk, tr[i])) continue;
		l = 0;
forward:
		if(!(c = choose_column())) goto backtrack;
		cover_column(c);
		r = O[l] = c->down;
advance:
		if(r == c) goto backup;
		if(l == n - 1){
			for(int j = 0, t, r1, r2, c1, c2; j < n; j++){
				t = O[j]->column >> 7;
				r1 = t >> 12;
				r2 = (t >> 8) & 0xf;
				c1 = (t >> 4) & 0xf;
				c2 = t & 0xf;
				tempt[r1] = c2;
				tempt[r2] = c1;
			}
			postr(dlk, lk, (const data**)O);
			kanonizator::kanon(dlk, kf);
			if(flag){
				flag = false;
				if(!baza_kf.insert(kf).second) return;
				kf_trans[srez].push_back(make_pair(kf, make_pair(tr[i], tempt)));
			}
			else if(baza_kf.insert(kf).second) kf_trans[srez].push_back(make_pair(kf, make_pair(tr[i], tempt)));
			goto recover;
		}
		for(data* j = r->right; j != r; j = j->right) cover_column(heads + (j->column & 0x7f));
		l++;
		goto forward;
backup:
		uncover_column(c);
backtrack:
		if(l == 0) continue;
		r = O[--l];
		c = heads + (r->column & 0x7f);
		for(data* j = r->left; j != r; j = j->left) uncover_column(heads + (j->column & 0x7f));
recover:
		r = O[l] = r->down;
		goto advance;
	}
}

bool trans_dlx::init_simm(const kvadrat& lk, const transver& tr){
	for(int i = 0; i < ch_cols_simm; i++){
		heads[i].down = heads[i].up = heads + i;
		heads[i].size = 0;
	}
	transver otr;
	for(int i = 0; i < por; i++) otr[tr[i]] = i;
	int count = 0, temp[8], r1, c1 = tr[0], r2, c2, rb = 0, cb = 0, v1b = 0, v2b = 0;
	for(c2 = c1 + 1; c2 < por; c2++){
		r2 = otr[c2];
		if((temp[4] = lk[c1]) == (temp[5] = lk[(r2 << 3) + (r2 << 1) + c2]) ||
			(temp[6] = lk[c2]) == (temp[7] = lk[(r2 << 3) + (r2 << 1) + c1])) continue;
		rb |= 1 | (1 << r2); cb |= (1 << c1) | (1 << c2);
		v1b |= (1 << temp[4]) | (1 << temp[5]); v2b |= (1 << temp[6]) | (1 << temp[7]);
		temp[0] = 1;
		temp[1] = r2 + 1;
		temp[2] = c1 + por + 1;
		temp[3] = c2 + por + 1;
		temp[4] += 2 * por + 1;
		temp[5] += 2 * por + 1;
		temp[6] += 3 * por + 1;
		temp[7] += 3 * por + 1;
		for(int i = 0, t; i < 8; i++){
			nodes[count + i].column = (r2 << 15) | (c1 << 11) | (c2 << 7) | (t = temp[i]);
			nodes[count + i].right = nodes.data() + count + i + 1;
			nodes[count + i].left = nodes.data() + count + i - 1;
			nodes[count + i].down = heads + t;
			nodes[count + i].up = heads[t].up;
			heads[t].up = heads[t].up->down = nodes.data() + count + i;
			heads[t].size++;
		}
		nodes[count].left += 8;
		nodes[count + 7].right -= 8;
		count += 8;
	}
	if(!(rb & 1)) return false;
	for(r1 = 1; r1 < por - 1; r1++){
		for(r2 = r1 + 1; r2 < por; r2++){
			c1 = tr[r1]; c2 = tr[r2];
			if((temp[4] = lk[(r1 << 3) + (r1 << 1) + c1]) == (temp[5] = lk[(r2 << 3) + (r2 << 1) + c2]) ||
				(temp[6] = lk[(r1 << 3) + (r1 << 1) + c2]) == (temp[7] = lk[(r2 << 3) + (r2 << 1) + c1])) continue;
			rb |= (1 << r1) | (1 << r2); cb |= (1 << c1) | (1 << c2);
			v1b |= (1 << temp[4]) | (1 << temp[5]); v2b |= (1 << temp[6]) | (1 << temp[7]);
			temp[0] = r1 + 1;
			temp[1] = r2 + 1;
			temp[2] = c1 + por + 1;
			temp[3] = c2 + por + 1;
			temp[4] += 2 * por + 1;
			temp[5] += 2 * por + 1;
			temp[6] += 3 * por + 1;
			temp[7] += 3 * por + 1;
			for(int i = 0, t; i < 8; i++){
				nodes[count + i].column = (r1 << 19) | (r2 << 15) | (c1 << 11) | (c2 << 7) | (t = temp[i]);
				nodes[count + i].right = nodes.data() + count + i + 1;
				nodes[count + i].left = nodes.data() + count + i - 1;
				nodes[count + i].down = heads + t;
				nodes[count + i].up = heads[t].up;
				heads[t].up = heads[t].up->down = nodes.data() + count + i;
				heads[t].size++;
			}
			nodes[count].left += 8;
			nodes[count + 7].right -= 8;
			count += 8;
		}
	}
	return !((rb & cb & v1b & v2b) ^ 0x3ff);
}

void trans_dlx::search_symm_trans(const kvadrat* srez[]){
	baza_kf.clear();
	for(int i = 0; i < ch_srez; i++) kf_trans[i].clear();
	for(int i = 0; i < ch_srez; i++){
		if(cnt_trans < 1000) poisk_simm(*srez[i], trans[i], i);
		else poisk_simm_dlx(*srez[i], trans[i], i);
	}
}

bool trans_dlx::is_mar(){
	init_mar();
	data* c;
	data* r;
	int l = 0;
forward:
	if(!(c = choose_column())) goto backtrack;
	cover_column(c);
	r = O[l] = c->down;
advance:
	if(r == c) goto backup;
	if(l == por - 1) return true;
	for(data* j = r->right; j != r; j = j->right) cover_column(heads + (j->column & 0x7f));
	l++;
	goto forward;
backup:
	uncover_column(c);
backtrack:
	if(l == 0) return false;
	r = O[--l];
	c = heads + (r->column & 0x7f);
	for(data* j = r->left; j != r; j = j->left) uncover_column(heads + (j->column & 0x7f));
	r = O[l] = r->down;
	goto advance;
}

void trans_dlx::init_mar(){
	if(d_trans.size() > max_trans){
		nodes.clear();
		nodes.resize(d_trans.size() * por);
	}
	for(int i = 0; i < max_cols; i++){
		heads[i].right = heads + i + 1;
		heads[i].left = heads + i - 1;
		heads[i].down = heads[i].up = heads + i;
		heads[i].size = 0;
	}
	heads[0].left += max_cols;
	heads[raz].right = heads;
	int count = 0;
	for(auto q = d_trans.begin(); q != d_trans.end(); q++){
		for(int j = 0, k = 0, t; j < por; k += por, j++){
			nodes[count + j].column = t = (*q)[j] + k + 1;
			nodes[count + j].right = nodes.data() + count + j + 1;
			nodes[count + j].left = nodes.data() + count + j - 1;
			nodes[count + j].down = heads + t;
			nodes[count + j].up = heads[t].up;
			heads[t].up = heads[t].up->down = nodes.data() + count + j;
			heads[t].size++;
		}
		nodes[count].left += por;
		nodes[count + por - 1].right -= por;
		count += por;
	}
}
