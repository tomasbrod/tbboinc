/* exact_cover_u.cpp */

class Exact_cover_u {
	public:
	typedef unsigned short elem_t;

	unsigned order; // size of the LK

	std::vector<elem_t> transversals;
	size_t num_trans;
	elem_t* add_trans();
	const elem_t* get_trans(size_t i) const { return &transversals[order*i]; }
	std::vector<Square> mates;

	void search_trans(const Square& dlk)
	{
		order= dlk.width();
		raz = order * order;
		max_cols = raz + 1;
		transversals.clear();
		mates.clear();
		O.resize(order);
		heads.resize(max_cols);
		nodes.resize((4*raz));
		num_trans=0;
		init_trans(dlk);
		stop_dance=false;
		dance(&Exact_cover_u::save_trans);
	}

	void search_mates()
	{
		init_disjoint();
		stop_dance=false;
		dance(&Exact_cover_u::save_mates);
	}


	protected:
	void init_trans(const Square& dlk);
	void init_disjoint();

	struct data{
		data* left;
		data* right;
		data* up;
		data* down;
		union{
			int column;
			int size;
		};
		int dr, dc;
	};

	int raz;
	int max_cols;

	std::vector<data*> O;   //order -> dataNodeSt
	std::vector<data> heads;
	std::vector<data> nodes;

	data* choose_column(){
		data* c = heads[0].right;
		int s = c->size;
		if(!s) return c;
		for(data* j = c->right; j != &heads[0]; j = j->right) if(j->size < s){
			c = j;
			if(!(s = j->size)) return c;
		}
		return c;
	}

	void cover_column(data* p){
		p->right->left = p->left;
		p->left->right = p->right;
		for(data* i = p->down; i != p; i = i->down){
			for(data* j = i->right; j != i; j = j->right){
				j->down->up = j->up;
				j->up->down = j->down;
				heads[j->column & 0x7f].size--;
			}
		}
	}

	void uncover_column(data* p){
		for(data* i = p->up; i != p; i = i ->up){
			for(data* j = i->left; j != i; j = j->left){
				heads[j->column & 0x7f].size++;
				j->down->up = j->up->down = j;
			}
		}
		p->right->left = p->left->right = p;
	}

	typedef  void (Exact_cover_u::*save_func_t)(data** O);
	bool stop_dance;
	void save_trans(data** O);
	void save_one_mate(data** O);
	void save_mates(data** O);
	void dance(save_func_t save);
};

void Exact_cover_u::save_mates(data** O) {
	Square mate(order);
	for(int i = 0; i < order; i++){
		for(int j = 0, k = 0; j < order; k += order, j++){
			mate[k + get_trans(O[i]->dr)[j]] = i;
		}
	}
	mates.push_back(mate);
}


void Exact_cover_u::init_trans(const Square& dlk){
	const int nheads = order * 3 + 2;
	for(int i = 0; i <= nheads; i++){
		heads[i].right = &heads[i + 1];
		heads[i].left = &heads[i - 1];
		heads[i].down = heads[i].up = &heads[i];
		heads[i].size = 0;
	}
	heads[0].left = &heads[nheads];
	heads[nheads].right = &heads[0];
	int temp[4];
	int i,count,r,c;
	for(i = 0, count = 0, r, c; i < raz; i++){
		temp[0] = (r = i / order) + 1;
		temp[1] = (c = i % order) + order + 1;
		temp[2] = dlk[i] + (order << 1) + 1;
		temp[3] = r == c ? (nheads-1) : r == order - 1 - c ? nheads : 0;
		for(int j = 0, t; j < 3; j++){
			nodes[count + j].column = t = temp[j];
			nodes[count + j].dr = r;
			nodes[count + j].dc = c;
			nodes[count + j].right = &nodes[count + j + 1];
			nodes[count + j].left = &nodes[count + j - 1];
			nodes[count + j].down = &heads[t];
			nodes[count + j].up = heads[t].up;
			heads[t].up = heads[t].up->down = &nodes[count + j];
			heads[t].size++;
		}
		if(int t = temp[3]){
			nodes[count + 3].column = t;
			nodes[count + 3].dr = r;
			nodes[count + 3].dc = c;
			nodes[count + 3].right = &nodes[count];
			nodes[count + 3].left = &nodes[count + 2];
			nodes[count + 3].down = &heads[t];
			nodes[count + 3].up = heads[t].up;
			heads[t].up = heads[t].up->down = &nodes[count + 3];
			heads[t].size++;
			nodes[count].left += 4;
			count += 4;
		}
		else{
			nodes[count].left += 3;
			nodes[count + 2].right -= 3;
			count += 3;
		}
	}
	std::cerr<<"init_trans("<<dlk.width()<<") used "<<count<<" nodes\n";
}

Exact_cover_u::elem_t* Exact_cover_u::add_trans()
{
	size_t pos = num_trans*order;
	transversals.resize(pos+order);
	num_trans++;
	return &transversals[pos];
}

void Exact_cover_u::save_trans(data** O)
{
	elem_t* tempt;
	tempt = add_trans();
	for(int i = 0, t; i < order; i++){
		tempt[O[i]->dr]=O[i]->dc;
	}
}

void Exact_cover_u::dance(save_func_t save){
	data* c;
	data* r;
	int l = 0;
enter_level_l: 
	if(l >= order){
		(this->*save)(O.data());
		if(stop_dance)
			return;
		goto backtrack;
	}
	c = choose_column();
	r = c->down;
try_to_advance:
	if(r != c){
		cover_column(c);
		for(data* j = r->right; j != r; j = j->right) cover_column(&heads[j->column & 0x7f]);
		O[l++] = r;
		goto enter_level_l;
	}
backtrack:
	if(--l >= 0){
		r = O[l];
		c = &heads[r->column & 0x7f];
		for(data* j = r->left; j != r; j = j->left) uncover_column(&heads[j->column & 0x7f]);
		uncover_column(c);
		r = r->down;
		goto try_to_advance;
	}
	return;
}

void Exact_cover_u::init_disjoint(){
	const int ch_trans = num_trans;
	const int max_nodes = ch_trans * order;
	nodes.resize(max_nodes);
	for(int i = 0; i <= raz; i++){
		heads[i].right = &heads[i + 1];
		heads[i].left = &heads[i - 1];
		heads[i].down = heads[i].up = &heads[i];
		heads[i].size = 0;
	}
	heads[0].left = &heads[raz];
	heads[raz].right = &heads[0];
	int i, count;
	for(i = 0, count = 0; i < ch_trans; count += order, i++){
		for(int j = 0, k = 0, t; j < order; k += order, j++){
			nodes[count + j].column = t = get_trans(i)[j] + k + 1;
			nodes[count + j].dr = i;
			nodes[count + j].right = &nodes[count + j + 1];
			nodes[count + j].left = &nodes[count + j - 1];
			nodes[count + j].down = &heads[t];
			nodes[count + j].up = heads[t].up;
			heads[t].up = heads[t].up->down = &nodes[count + j];
			heads[t].size++;
		}
		nodes[count].left += order;
		nodes[count + order - 1].right -= order;
	}
	std::cerr<<"init_disjoint("<<order<<") used "<<(raz+1)<<" heads and "<<count<<" nodes\n";

}

