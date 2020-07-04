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
		init_trans(dlk);
		dance(&Exact_cover_u::save_trans);
	}

	size_t count_trans(const Square& dlk)
	{
		init_trans(dlk);
		dance_mt(&Exact_cover_u::save_count_trans);
    return num_trans;
	}

	bool is_ortogon()
	{
		init_disjoint();
		dance(&Exact_cover_u::save_one_mate);
		return !mates.empty();
	}

	void search_mates()
	{
		init_disjoint();
		stop_dance=false;
		dance_mt(&Exact_cover_u::save_mates);
	}


	protected:
	void init_trans(const Square& dlk);
	int search_trans();
	void init_disjoint();
	bool search_disjoint(std::list<Square>* mates);

	typedef unsigned nodeix;

	struct dataComDyn {
		nodeix up;
		nodeix down;
	};
	struct dataHeadDyn {
		nodeix left;
		nodeix right;
		int size;
	};
	struct dataNodeSt {
		nodeix left;
		nodeix right;
		nodeix column;
		unsigned dr, dc;
	};

	int raz;
	int max_cols;

	std::vector<nodeix> O2;   //order -> dataNodeSt
	std::vector<dataHeadDyn> headsDyn;
	std::vector<dataComDyn> nodesDyn;
	std::vector<dataNodeSt> nodesSt;

	nodeix choose_column(dataHeadDyn* H){
		nodeix c = H[0].right;
		int s = H[c].size;
		if(!s) return c;
		for(nodeix j = H[c].right; j != 0; j = H[j].right) if(H[j].size < s){
			c = j;
			if(!(s = H[j].size)) return c;
		}
		return c;
	}
	/* which links changd and used
		Head LR: 1
		Head UD: 1
		node UD: 1
		node LR: 0
		Head dt: 0 N
		node dt: 0 Y
		node co: 0
		head sz: 1
	*/

	void cover_column(nodeix c, dataHeadDyn* H, dataComDyn* N){ //c:head
		H[H[c].right].left = H[c].left;
		H[H[c].left].right = H[c].right;
		for(nodeix i = N[c].down; i != c; i = N[i].down){ //i:node
			for(nodeix j = nodesSt[i].right; j != i; j = nodesSt[j].right){ //j:node
				N[N[j].down].up = N[j].up; //j->down: node or head
				N[N[j].up].down = N[j].down;
				H[nodesSt[j].column].size--;
			}
		}
	}

	void uncover_column(nodeix c, dataHeadDyn* H, dataComDyn* N){
		for(nodeix i = N[c].up; i != c; i = N[i].up){ //i:node
			for(nodeix j = nodesSt[i].left; j != i; j = nodesSt[j].left){ //j:node
				H[nodesSt[j].column].size++;
				N[N[j].down].up = N[N[j].up].down = j; //j->down: node or head
			}
		}
		H[H[c].right].left = H[H[c].left].right = c;
	}

	typedef  void (Exact_cover_u::*save_func_t)(nodeix* O);
	bool stop_dance;
	void save_trans(nodeix* O);
	void save_one_mate(nodeix* O);
	void save_mates(nodeix* O);
	void dance(save_func_t save, dataHeadDyn* H, dataComDyn* N, nodeix* O, int level);
	void dance(save_func_t save) { dance(save, headsDyn.data(),nodesDyn.data(), O2.data(), 0); }
	void dance_mt(save_func_t save);
	std::mutex cs_main;
	nodeix global_r;
	size_t global_siz;
	size_t global_cnt;
	save_func_t global_save;
	void dance_mt_thr(bool show);
	void connect_node(nodeix ix, nodeix col);
	void dance_lim(save_func_t save);
	void save_count_trans(nodeix* O);
};

void Exact_cover_u::save_one_mate(nodeix* O){
	Square mate(order);
	for(int i = 0; i < order; i++){
		for(int j = 0, k = 0; j < order; k += order, j++){
			mate[k + get_trans(nodesSt[O[i]].dr)[j]] = i; //O[i]:node
		}
	}
	std::lock_guard<std::mutex> lock(cs_main);
	mates.push_back(mate);
	stop_dance=true;
}
void Exact_cover_u::save_mates(nodeix* O) {
	Square mate(order);
	for(int i = 0; i < order; i++){
		for(int j = 0, k = 0; j < order; k += order, j++){
			mate[k + get_trans(nodesSt[O[i]].dr)[j]] = i; //O[i]:node
		}
	}
	std::lock_guard<std::mutex> lock(cs_main);
	mates.push_back(mate);
}

void Exact_cover_u::connect_node(nodeix ix, nodeix col)
{
	nodesSt[ix].column = col;
	nodesSt[ix].right = ix + 1;
	nodesSt[ix].left = ix - 1;
	nodesDyn[ix].down = col;
	nodesDyn[ix].up = nodesDyn[col].up;
	nodesDyn[col].up = nodesDyn[nodesDyn[col].up].down = ix;
	headsDyn[col].size++;
}  


void Exact_cover_u::init_trans(const Square& dlk){
  order= dlk.width();
  raz = order * order;
	const int nheads = order * 3 + 2;
  max_cols = nheads + 1;
  transversals.clear();
  mates.clear();
  O2.resize(order);
  headsDyn.resize(max_cols);
  nodesDyn.resize(max_cols+(4*raz));
  nodesSt.resize(max_cols+(4*raz));
  num_trans=0;
  stop_dance=false;
	for(int i = 0; i <= nheads; i++){
		headsDyn[i].right = i + 1;
		headsDyn[i].left = i - 1;
		nodesDyn[i].down = nodesDyn[i].up = i;
		headsDyn[i].size = 0;
	}
	headsDyn[0].left = nheads;
	headsDyn[nheads].right = 0;
	int temp[3];
	int i,count,r,c,j;
	for(i = 0, count = nheads+1, r, c; i < raz; i++) {
		temp[0] = (r = i / order) + 1;
		temp[1] = (c = i % order) + order + 1;
		temp[2] = dlk[i] + (order << 1) + 1;
		for(j = 0; j < 3; j++){
			connect_node(count+j, temp[j]);
			nodesSt[count + j].dr = r;
			nodesSt[count + j].dc = c;
		}
		if(r == c) {
			connect_node(count+j, nheads-1);
			nodesSt[count + j].dr = r;
			nodesSt[count + j].dc = c;
			j++;
		}
		if(r == order - 1 - c) {
			connect_node(count+j, nheads);
			nodesSt[count + j].dr = r;
			nodesSt[count + j].dc = c;
			j++;
		}
		nodesSt[count].left += j;
		nodesSt[count + j - 1].right -= j;
		count += j;
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

void Exact_cover_u::save_trans(nodeix* O)
{
	elem_t* tempt;
	cs_main.lock();
	tempt = add_trans();
	for(int i = 0, t; i < order; i++){
		tempt[nodesSt[O[i]].dr]=nodesSt[O[i]].dc;
	}
	cs_main.unlock();
}

void Exact_cover_u::save_count_trans(nodeix* O)
{
	cs_main.lock();
	num_trans++;
	cs_main.unlock();
}

void Exact_cover_u::dance(save_func_t save, dataHeadDyn* H, dataComDyn* N, nodeix* O, int level){
	nodeix c;
	nodeix r;
	const int r_l = 5;
	unsigned r_num[r_l] = {0};
	unsigned r_cnt[r_l] = {101,0};
	int l = level;
enter_level_l: 
	if(l >= order){
		(this->*save)(O);
		if(stop_dance)
			return;
		goto backtrack;
	}
	c = choose_column(H);
	r = N[c].down;
	#if 0
	if(l<=r_l) {
		r_num[l]=0;
		if(l>0) r_num[l-1]++;
		r_cnt[l]=headsDyn[c].size;
		if(l>0 && r_cnt[l-1]>100) {
			std::cerr<<"          \r";
			for(unsigned i=1; i<r_l && r_cnt[i]>100; ++i)
				std::cerr<<"l("<<i<<") "<<r_num[i]<<" / "<<r_cnt[i]<<" s"<<(mates.size());
			std::cerr.flush();
		}
	}
	#endif
try_to_advance:
	if(r != c){
		cover_column(c,H,N);
		for(nodeix j = nodesSt[r].right; j != r; j = nodesSt[j].right)
			cover_column(nodesSt[j].column, H, N);
		O[l++] = r;
		goto enter_level_l;
	}
backtrack:
	if(--l >= level){
		r = O[l];
		c = nodesSt[r].column;
		for(nodeix j = nodesSt[r].left; j != r; j = nodesSt[j].left)
			uncover_column(nodesSt[j].column, H, N);
		uncover_column(c, H, N);
		r = N[r].down;
		goto try_to_advance;
	}
	return;
}

void Exact_cover_u::init_disjoint(){
  stop_dance=false;
	const int max_cols = raz+1;
	const int max_nodes = num_trans * order;
  headsDyn.resize(max_cols);
	nodesDyn.resize(max_cols+max_nodes);
	nodesSt.resize(max_cols+max_nodes);
	for(int i = 0; i <= raz; i++){
		headsDyn[i].right = i + 1;
		headsDyn[i].left = i - 1;
		nodesDyn[i].down = nodesDyn[i].up = i;
		headsDyn[i].size = 0;
	}
	headsDyn[0].left += max_cols;
	headsDyn[raz].right = 0;
	int i, count;
	for(i = 0, count = raz+1; i < num_trans; count += order, i++){
		for(int j = 0, k = 0, t; j < order; k += order, j++){
			connect_node(count+j, get_trans(i)[j] + k + 1);
			nodesSt[count + j].dr = i;
		}
		nodesSt[count].left += order;
		nodesSt[count + order - 1].right -= order;
	}
	std::cerr<<"init_disjoint("<<order<<") used "<<(raz+1)<<" heads and "<<count<<" nodes\n";
}

void Exact_cover_u::dance_mt_thr(bool show) {
	std::vector<dataHeadDyn> heads(headsDyn);
	std::vector<dataComDyn> nodes(nodesDyn);
	std::vector<nodeix> O(order);
	//note: The choosen column is already covered
	cs_main.lock();
	while(1) {
		nodeix r = global_r;
		if(r>0) {
			global_r = nodesDyn[r].down;
			cs_main.unlock();

			for(nodeix j = nodesSt[r].right; j != r; j = nodesSt[j].right)
				cover_column(nodesSt[j].column, &heads[0],&nodes[0]);
			O[0] = r;
			dance(global_save, &heads[0],&nodes[0],&O[0],1);
			for(nodeix j = nodesSt[r].left; j != r; j = nodesSt[j].left)
				uncover_column(nodesSt[j].column, &heads[0],&nodes[0]);

			cs_main.lock();
			size_t cnt = ++global_cnt;
			std::cerr<<"\rl(1) "<< cnt <<" / "<<global_siz<<"   ";
			std::cerr.flush();
			//continue;
			return;
		}
		cs_main.unlock();
		return;
	}
}

void Exact_cover_u::dance_mt(save_func_t save)
{
	nodeix c = choose_column(headsDyn.data());
	cover_column(c, headsDyn.data(),nodesDyn.data());
	nodesDyn[nodesDyn[c].up].down = 0;
	global_r = nodesDyn[c].down;
	global_save = save;
	global_siz = headsDyn[c].size;
	global_cnt=0;
	if(!global_siz) return;
	std::vector<std::thread> threads;
	size_t nthreads = std::min(size_t(std::thread::hardware_concurrency()), global_siz);
	std::cerr<<"dance_mt: using "<<nthreads<<" threads for "<<(headsDyn[c].size)<<" rows in column "<<c<<endl<<"...";
	std::cerr.flush();
	for(size_t tid=0; tid< nthreads-1; ++tid) {
		threads.emplace_back( &Exact_cover_u::dance_mt_thr, this, 0 );
	}
	dance_mt_thr(1);
	for( auto& thr : threads )
		thr.join();
	std::cerr<<endl;
}

void Exact_cover_u::dance_lim(save_func_t save)
{
	nodeix c = choose_column(headsDyn.data());
	cover_column(c, headsDyn.data(),nodesDyn.data());
	nodesDyn[nodesDyn[c].up].down = 0;
  nodeix r = nodesDyn[c].down;
  size_t cnt = 0;
	global_siz = headsDyn[c].size;
  size_t sols = 0;
	std::cerr<<"dance_lim: for "<<global_siz<<" rows in column "<<c<<endl;
  while(r) {

      for(nodeix j = nodesSt[r].right; j != r; j = nodesSt[j].right)
				cover_column(nodesSt[j].column, &headsDyn[0],&nodesDyn[0]);
			O2[0] = r;
			dance(save, &headsDyn[0],&nodesDyn[0],&O2[0],1);
			for(nodeix j = nodesSt[r].left; j != r; j = nodesSt[j].left)
				uncover_column(nodesSt[j].column, &headsDyn[0],&nodesDyn[0]);

    cnt++;
    std::cerr<<"l(1) "<< cnt <<" / "<<global_siz<<" r"<<r<<" s"<<(mates.size()-sols)<<endl;
    sols=mates.size();
    r= nodesDyn[r].down;
  }
}

#if 0
struct Ctest : Exact_cover_u {
  void save_test(nodeix* O)
  {
    cs_main.lock();
    num_trans++;
    cs_main.unlock();
  }
  void test() {
    dance(&Ctest::save_test); // this does not work due to stupid c++ pointers
  }
};
#endif
