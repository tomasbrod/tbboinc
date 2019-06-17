struct Generator {
	private:
	static const int n = por * (por - 2);
	static const unsigned all = (1 << por) - 1;
	static const int ch_lin = 67;

	typedef std::array<unsigned char,por> morfizm;
	static const int ch_klass2 = 56;
	static const int raz_trans_tabl = 114;
	static const int max_chform = 2;
	static const int trans_tabl[raz_trans_tabl][por << 1];
	static const int ukaz[ch_lin];
	static const int dop_trans[ch_klass2][por];

	kvadrat formy[max_chform];
	int chform;

	public:
	static const int diag[ch_lin][por];
	int min_l, max_l;
	int X[n], S[n], l, col[por], row[por], lin;
	kvadrat& dlk = formy[0];

	bool init(int lin, const array<int,80>& start)
	{
		this->lin= lin;
		l= min_l= 0;
		max_l= n;
		for(int i = 0; i < por; i++){
			row[i] = (1 << i) | (1 << diag[lin][i]);
			col[i] = (1 << i) | (1 << diag[lin][por - 1 - i]);
		}
		for(int x, s, r, c; l < 80; l++){
			x = 1 << start[l];
			s = (row[r = rc[l] >> 4] | col[c = rc[l] & 0xf]) ^ all;
			if(!(s & x)) break;
			X[l] = x;
			row[r] |= x;
			col[c] |= x;
			S[l] = s & -x;
		}
		if(l==n) {
			get_dlk(formy[0]);
			return true;
		}
		return false;
	}

	bool init(int lin, const kvadrat& start) {
		array<int,80> tmp;
		int raz_st=0;
		for(int k = 0; k < raz;++k){
			int i = k / por;
			int j = k % por;
			if(i == j || j == por - 1 - i) continue;
			tmp[raz_st++] = start[k];
		}
		return init(lin,tmp);
	}

	__attribute__((noinline)) bool next ()
	{
		if(l<max_l) goto enter_level_l;
		goto backtrack;

		enter_level_l: 
			if(l >= max_l){
				get_dlk(formy[0]);
				return true;
			}
			S[l] = (row[rc[l] >> 4] | col[rc[l] & 0xf]) ^ all;
			goto try_to_advance;
		try_to_advance:
			if(S[l]){
				X[l] = (S[l] & (-S[l]));
				update_downdate(l++);
				goto enter_level_l;
			}
			goto backtrack;
		backtrack:
			if(--l >= min_l){
				update_downdate(l);
				S[l] ^= X[l];
				goto try_to_advance;
			}
			return false;
	}
	
	__attribute__((noinline)) bool is_kf () {
		if(lin < ch_klass2){
			chform = 2;
			simmetr();
			if(formy[1] < formy[0]) return false;
		}
		else chform = 1;
		int kol = ukaz[lin] & 0x1f, ind = ukaz[lin] >> 5;
		for(int i = 0; i < kol; i++)
			if(!obrabotka(&trans_tabl[ind + i][0], &trans_tabl[ind + i][por])) return false;
		return true;
	}
	
	bool is_kf(const kvadrat& lk) {
		formy[0]= lk;
		return is_kf();
	}

	private:
	bool obrabotka(const int* perest, const int* ob_perest) {
		int x, minz, index = 0, count[2], spisok[2][max_chform];
		for(int i = 0; i < chform; i++) spisok[0][i] = i;
		count[0] = chform;
		for(int i = 0; i < por - 1; i++) for(int j = 0; j < por - 1; j++){
			if(j == i || j == por - 1 - i) continue;
			minz = formy[0][i * por + j];
			count[index ^ 1] = 0;
			for(int k = 0; k < count[index]; k++){
				if(minz > (x = ob_perest[formy[spisok[index][k]][perest[i] * por + perest[j]]])){
					return false;
				}
				else if(minz == x) spisok[index ^ 1][count[index ^ 1]++] = spisok[index][k];
			}
			if(!count[index ^= 1]) return true;
		}
		return true;
	}
	void simmetr() {
		kvadrat tempk = formy[0];
		for(int i = 0; i < por; i++) for(int j = 0; j < (por >> 1); j++)
			std::swap(tempk[i * por + j], tempk[i * por + por - 1 - j]);
		for(int i = 0; i < por; i++) for(int j = 0; j < por; j++)
			formy[1][i * por + j] = tempk[dop_trans[lin][i] * por + dop_trans[lin][j]];
		morfizm perest;
		for(int i = 0, j = 0; i < por; j += por + 1, i++) perest[formy[1][j]] = i;
		for(int i = 0; i < raz; i++) formy[1][i] = perest[formy[1][i]];
	}
	static const int rc[n];
	void get_dlk(kvadrat& tempk) {
		for(int i = 0; i < por; i++){
			tempk[i * (por + 1)] = i;
			tempk[ (i + 1) * (por - 1)] = diag[lin][i];
		}
		for(int i = 0, t; i < l; i++){
			t = 0;
			if(X[i] & 0x300) t += 8;
			if(X[i] & 0xf0)  t += 4;
			if(X[i] & 0xcc)  t += 2;
			if(X[i] & 0x2aa) t += 1;
			tempk[(rc[i] >> 4) * por + (rc[i] & 0xf)] = t;
		}
	}
	void update_downdate(int l){
		int t = rc[l];
		int x = X[l];
		row[t >> 4] ^= x;
		col[t & 0xf] ^= x;
	}
};

const int Generator::diag[ch_lin][por] = {
		1,0,3,2,6,7,4,5,9,8,
		1,0,3,2,6,7,4,8,9,5,
		1,0,3,2,6,7,5,4,9,8,
		1,0,3,2,6,7,8,9,4,5,
		1,0,3,2,6,7,8,9,5,4,
		1,0,3,2,6,8,4,9,5,7,
		1,0,3,2,6,8,4,9,7,5,
		1,0,3,2,6,8,5,9,4,7,
		1,0,3,2,6,8,5,9,7,4,
		1,0,3,2,6,8,7,4,9,5,
		1,0,3,2,6,8,9,4,7,5,
		1,0,3,2,6,8,9,5,7,4,
		1,0,3,4,2,6,8,9,5,7,
		1,0,3,4,2,6,8,9,7,5,
		1,0,3,4,2,7,5,6,9,8,
		1,0,3,4,2,7,5,8,9,6,
		1,0,3,4,2,7,8,9,5,6,
		1,0,3,4,2,7,8,9,6,5,
		1,0,3,4,6,2,5,8,9,7,
		1,0,3,4,6,2,8,5,9,7,
		1,0,3,4,6,2,8,9,5,7,
		1,0,3,4,6,8,2,9,7,5,
		1,0,3,4,6,8,5,9,2,7,
		1,0,3,4,6,8,7,9,2,5,
		1,0,3,4,6,8,7,9,5,2,
		1,0,3,4,7,2,8,9,5,6,
		1,0,3,4,7,2,8,9,6,5,
		1,0,3,4,7,8,5,9,2,6,
		1,0,3,4,7,8,5,9,6,2,
		1,0,3,4,8,7,5,9,2,6,
		1,0,3,4,8,7,5,9,6,2,
		1,0,3,4,8,9,5,6,2,7,
		1,0,3,4,8,9,5,6,7,2,
		1,0,3,7,6,8,5,9,2,4,
		1,0,3,7,6,8,5,9,4,2,
		1,0,3,7,8,9,2,6,4,5,
		1,0,3,7,8,9,2,6,5,4,
		1,2,0,4,6,3,5,9,7,8,
		1,2,0,4,6,3,7,9,5,8,
		1,2,0,4,6,7,8,9,3,5,
		1,2,0,4,7,8,5,9,3,6,
		1,2,0,4,7,8,5,9,6,3,
		1,2,0,4,7,8,9,3,6,5,
		1,2,0,4,7,8,9,5,6,3,
		1,2,0,4,7,9,8,5,3,6,
		1,2,0,4,7,9,8,6,5,3,
		1,2,3,0,6,7,8,9,5,4,
		1,2,3,0,6,7,9,4,5,8,
		1,2,3,4,0,7,5,9,6,8,
		1,2,3,4,0,7,8,9,5,6,
		1,2,3,4,0,9,5,6,7,8,
		1,2,3,4,6,0,8,9,7,5,
		1,2,3,4,6,7,5,9,0,8,
		1,2,3,4,6,8,9,5,0,7,
		1,2,3,7,6,8,5,9,0,4,
		1,2,3,7,6,9,5,4,0,8,
		1,0,3,2,6,7,5,8,9,4,
		1,0,3,4,6,2,8,9,7,5,
		1,0,3,4,6,7,8,9,2,5,
		1,0,3,4,6,7,8,9,5,2,
		1,0,3,4,6,8,5,9,7,2,
		1,0,3,4,6,8,9,5,2,7,
		1,0,3,4,6,8,9,5,7,2,
		1,0,3,4,8,6,9,5,2,7,
		1,2,0,4,6,7,8,9,5,3,
		1,2,3,0,6,7,5,9,4,8,
		1,2,3,4,6,9,8,0,5,7
};
const int Generator::rc[n] = {
		0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
		0x10,0x12,0x13,0x14,0x15,0x16,0x17,0x19,
		0x20,0x21,0x23,0x24,0x25,0x26,0x28,0x29,
		0x30,0x31,0x32,0x34,0x35,0x37,0x38,0x39,
		0x40,0x41,0x42,0x43,0x46,0x47,0x48,0x49,
		0x50,0x51,0x52,0x53,0x56,0x57,0x58,0x59,
		0x60,0x61,0x62,0x64,0x65,0x67,0x68,0x69,
		0x70,0x71,0x73,0x74,0x75,0x76,0x78,0x79,
		0x80,0x82,0x83,0x84,0x85,0x86,0x87,0x89,
		0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98
};

const int Generator::trans_tabl[][por << 1] = {
	0,1,3,2,5,4,7,6,8,9,0,1,3,2,5,4,7,6,8,9,
	1,0,2,3,4,5,6,7,9,8,1,0,2,3,4,5,6,7,9,8,
	1,0,3,2,5,4,7,6,9,8,1,0,3,2,5,4,7,6,9,8,
	8,9,2,3,4,5,6,7,0,1,8,9,2,3,4,5,6,7,0,1,
	8,9,3,2,5,4,7,6,0,1,8,9,3,2,5,4,7,6,0,1,
	9,8,2,3,4,5,6,7,1,0,9,8,2,3,4,5,6,7,1,0,
	9,8,3,2,5,4,7,6,1,0,9,8,3,2,5,4,7,6,1,0,
	1,0,7,5,6,3,4,2,9,8,1,0,7,5,6,3,4,2,9,8,
	0,1,4,6,2,7,3,5,8,9,0,1,4,6,2,7,3,5,8,9,
	0,1,5,7,3,6,2,4,8,9,0,1,6,4,7,2,5,3,8,9,
	0,1,6,4,7,2,5,3,8,9,0,1,5,7,3,6,2,4,8,9,
	0,1,7,5,6,3,4,2,8,9,0,1,7,5,6,3,4,2,8,9,
	1,0,4,6,2,7,3,5,9,8,1,0,4,6,2,7,3,5,9,8,
	1,0,5,7,3,6,2,4,9,8,1,0,6,4,7,2,5,3,9,8,
	1,0,6,4,7,2,5,3,9,8,1,0,5,7,3,6,2,4,9,8,
	8,9,4,6,2,7,3,5,0,1,8,9,4,6,2,7,3,5,0,1,
	8,9,5,7,3,6,2,4,0,1,8,9,6,4,7,2,5,3,0,1,
	8,9,6,4,7,2,5,3,0,1,8,9,5,7,3,6,2,4,0,1,
	8,9,7,5,6,3,4,2,0,1,8,9,7,5,6,3,4,2,0,1,
	9,8,4,6,2,7,3,5,1,0,9,8,4,6,2,7,3,5,1,0,
	9,8,5,7,3,6,2,4,1,0,9,8,6,4,7,2,5,3,1,0,
	9,8,6,4,7,2,5,3,1,0,9,8,5,7,3,6,2,4,1,0,
	9,8,7,5,6,3,4,2,1,0,9,8,7,5,6,3,4,2,1,0,
	1,0,4,6,2,7,3,5,9,8,1,0,4,6,2,7,3,5,9,8,
	2,3,0,1,5,4,8,9,6,7,2,3,0,1,5,4,8,9,6,7,
	9,7,8,5,6,3,4,1,2,0,9,7,8,5,6,3,4,1,2,0,
	3,2,5,8,0,9,1,4,7,6,4,6,1,0,7,2,9,8,3,5,
	4,6,1,0,7,2,9,8,3,5,3,2,5,8,0,9,1,4,7,6,
	5,8,3,2,9,0,7,6,1,4,5,8,3,2,9,0,7,6,1,4,
	6,4,7,9,1,8,0,2,5,3,6,4,7,9,1,8,0,2,5,3,
	7,9,6,4,8,1,5,3,0,2,8,5,9,7,3,6,2,0,4,1,
	8,5,9,7,3,6,2,0,4,1,7,9,6,4,8,1,5,3,0,2,
	0,1,3,7,4,5,2,6,8,9,0,1,6,2,4,5,7,3,8,9,
	0,1,6,2,4,5,7,3,8,9,0,1,3,7,4,5,2,6,8,9,
	0,1,7,6,4,5,3,2,8,9,0,1,7,6,4,5,3,2,8,9,
	1,0,2,3,5,4,6,7,9,8,1,0,2,3,5,4,6,7,9,8,
	1,0,3,7,5,4,2,6,9,8,1,0,6,2,5,4,7,3,9,8,
	1,0,6,2,5,4,7,3,9,8,1,0,3,7,5,4,2,6,9,8,
	1,0,7,6,5,4,3,2,9,8,1,0,7,6,5,4,3,2,9,8,
	9,5,7,6,8,1,3,2,4,0,9,5,7,6,8,1,3,2,4,0,
	4,8,2,3,0,9,6,7,1,5,4,8,2,3,0,9,6,7,1,5,
	4,8,3,7,0,9,2,6,1,5,4,8,6,2,0,9,7,3,1,5,
	4,8,6,2,0,9,7,3,1,5,4,8,3,7,0,9,2,6,1,5,
	4,8,7,6,0,9,3,2,1,5,4,8,7,6,0,9,3,2,1,5,
	5,9,2,3,1,8,6,7,0,4,8,4,2,3,9,0,6,7,5,1,
	5,9,3,7,1,8,2,6,0,4,8,4,6,2,9,0,7,3,5,1,
	5,9,6,2,1,8,7,3,0,4,8,4,3,7,9,0,2,6,5,1,
	5,9,7,6,1,8,3,2,0,4,8,4,7,6,9,0,3,2,5,1,
	8,4,2,3,9,0,6,7,5,1,5,9,2,3,1,8,6,7,0,4,
	8,4,3,7,9,0,2,6,5,1,5,9,6,2,1,8,7,3,0,4,
	8,4,6,2,9,0,7,3,5,1,5,9,3,7,1,8,2,6,0,4,
	8,4,7,6,9,0,3,2,5,1,5,9,7,6,1,8,3,2,0,4,
	9,5,2,3,8,1,6,7,4,0,9,5,2,3,8,1,6,7,4,0,
	9,5,3,7,8,1,2,6,4,0,9,5,6,2,8,1,7,3,4,0,
	9,5,6,2,8,1,7,3,4,0,9,5,3,7,8,1,2,6,4,0,
	1,2,3,4,0,9,5,6,7,8,4,0,1,2,3,6,7,8,9,5,
	2,3,4,0,1,8,9,5,6,7,3,4,0,1,2,7,8,9,5,6,
	3,4,0,1,2,7,8,9,5,6,2,3,4,0,1,8,9,5,6,7,
	4,0,1,2,3,6,7,8,9,5,1,2,3,4,0,9,5,6,7,8,
	5,9,8,7,6,3,2,1,0,4,8,7,6,5,9,0,4,3,2,1,
	6,5,9,8,7,2,1,0,4,3,7,6,5,9,8,1,0,4,3,2,
	7,6,5,9,8,1,0,4,3,2,6,5,9,8,7,2,1,0,4,3,
	8,7,6,5,9,0,4,3,2,1,5,9,8,7,6,3,2,1,0,4,
	9,8,7,6,5,4,3,2,1,0,9,8,7,6,5,4,3,2,1,0,
	0,1,3,4,2,7,5,6,8,9,0,1,4,2,3,6,7,5,8,9,
	0,1,4,2,3,6,7,5,8,9,0,1,3,4,2,7,5,6,8,9,
	0,1,5,7,6,3,2,4,8,9,0,1,6,5,7,2,4,3,8,9,
	0,1,6,5,7,2,4,3,8,9,0,1,5,7,6,3,2,4,8,9,
	0,1,7,6,5,4,3,2,8,9,0,1,7,6,5,4,3,2,8,9,
	1,0,2,3,4,5,6,7,9,8,1,0,2,3,4,5,6,7,9,8,
	1,0,3,4,2,7,5,6,9,8,1,0,4,2,3,6,7,5,9,8,
	1,0,4,2,3,6,7,5,9,8,1,0,3,4,2,7,5,6,9,8,
	1,0,5,7,6,3,2,4,9,8,1,0,6,5,7,2,4,3,9,8,
	1,0,6,5,7,2,4,3,9,8,1,0,5,7,6,3,2,4,9,8,
	1,0,7,6,5,4,3,2,9,8,1,0,7,6,5,4,3,2,9,8,
	8,9,2,3,4,5,6,7,0,1,8,9,2,3,4,5,6,7,0,1,
	8,9,3,4,2,7,5,6,0,1,8,9,4,2,3,6,7,5,0,1,
	8,9,4,2,3,6,7,5,0,1,8,9,3,4,2,7,5,6,0,1,
	8,9,5,7,6,3,2,4,0,1,8,9,6,5,7,2,4,3,0,1,
	8,9,6,5,7,2,4,3,0,1,8,9,5,7,6,3,2,4,0,1,
	8,9,7,6,5,4,3,2,0,1,8,9,7,6,5,4,3,2,0,1,
	9,8,2,3,4,5,6,7,1,0,9,8,2,3,4,5,6,7,1,0,
	9,8,3,4,2,7,5,6,1,0,9,8,4,2,3,6,7,5,1,0,
	9,8,4,2,3,6,7,5,1,0,9,8,3,4,2,7,5,6,1,0,
	9,8,5,7,6,3,2,4,1,0,9,8,6,5,7,2,4,3,1,0,
	9,8,6,5,7,2,4,3,1,0,9,8,5,7,6,3,2,4,1,0,
	9,8,7,6,5,4,3,2,1,0,9,8,7,6,5,4,3,2,1,0,
	0,1,2,4,6,3,5,7,8,9,0,1,2,5,3,6,4,7,8,9,
	0,1,2,5,3,6,4,7,8,9,0,1,2,4,6,3,5,7,8,9,
	0,1,2,6,5,4,3,7,8,9,0,1,2,6,5,4,3,7,8,9,
	1,2,0,3,4,5,6,9,7,8,2,0,1,3,4,5,6,8,9,7,
	1,2,0,4,6,3,5,9,7,8,2,0,1,5,3,6,4,8,9,7,
	1,2,0,5,3,6,4,9,7,8,2,0,1,4,6,3,5,8,9,7,
	1,2,0,6,5,4,3,9,7,8,2,0,1,6,5,4,3,8,9,7,
	2,0,1,3,4,5,6,8,9,7,1,2,0,3,4,5,6,9,7,8,
	2,0,1,4,6,3,5,8,9,7,1,2,0,5,3,6,4,9,7,8,
	2,0,1,5,3,6,4,8,9,7,1,2,0,4,6,3,5,9,7,8,
	2,0,1,6,5,4,3,8,9,7,1,2,0,6,5,4,3,9,7,8,
	7,9,8,3,4,5,6,1,0,2,8,7,9,3,4,5,6,0,2,1,
	7,9,8,4,6,3,5,1,0,2,8,7,9,5,3,6,4,0,2,1,
	7,9,8,5,3,6,4,1,0,2,8,7,9,4,6,3,5,0,2,1,
	7,9,8,6,5,4,3,1,0,2,8,7,9,6,5,4,3,0,2,1,
	8,7,9,3,4,5,6,0,2,1,7,9,8,3,4,5,6,1,0,2,
	8,7,9,4,6,3,5,0,2,1,7,9,8,5,3,6,4,1,0,2,
	8,7,9,5,3,6,4,0,2,1,7,9,8,4,6,3,5,1,0,2,
	8,7,9,6,5,4,3,0,2,1,7,9,8,6,5,4,3,1,0,2,
	9,8,7,3,4,5,6,2,1,0,9,8,7,3,4,5,6,2,1,0,
	9,8,7,4,6,3,5,2,1,0,9,8,7,5,3,6,4,2,1,0,
	9,8,7,5,3,6,4,2,1,0,9,8,7,4,6,3,5,2,1,0,
	9,8,7,6,5,4,3,2,1,0,9,8,7,6,5,4,3,2,1,0,
	6,8,7,9,5,4,0,2,1,3,6,8,7,9,5,4,0,2,1,3,
	5,8,6,7,9,0,2,3,1,4,5,8,6,7,9,0,2,3,1,4,
	6,5,7,9,8,1,0,2,4,3,6,5,7,9,8,1,0,2,4,3,
	6,8,5,9,7,2,0,4,1,3,6,8,5,9,7,2,0,4,1,3
};

const int Generator::ukaz[] = {
	23,0,7,65,65,745,737,769,0,769,
	769,769,801,0,2071,0,0,3521,0,0,
	0,737,0,0,3553,0,3521,0,0,0,0,
	1217,1217,737,737,1047,1031,2807,
	0,1249,0,3553,0,0,0,0,0,0,3585,
	1764,1769,3521,3585,0,0,0,0,0,0,
	0,0,0,0,225,0,0,3617
};

const int Generator::dop_trans[][por]  = {
	0,1,2,3,4,5,6,7,8,9,
	1,0,4,6,2,7,3,5,9,8,
	9,8,2,3,5,4,6,7,1,0,
	2,3,0,1,4,5,8,9,6,7,
	2,3,0,1,5,4,8,9,6,7,
	0,1,2,3,4,5,6,7,8,9,
	0,1,4,6,2,7,3,5,8,9,
	0,1,2,3,5,4,6,7,8,9,
	1,0,2,3,5,4,6,7,9,8,
	3,2,1,0,5,4,9,8,7,6,
	3,2,1,0,5,4,9,8,7,6,
	3,2,1,0,4,5,9,8,7,6,
	0,1,2,4,3,6,5,7,8,9,
	1,0,2,4,3,6,5,7,9,8,
	0,1,2,4,3,6,5,7,8,9,
	1,0,3,2,4,5,7,6,9,8,
	1,0,4,3,2,7,6,5,9,8,
	0,1,4,3,2,7,6,5,8,9,
	1,0,2,5,6,3,4,7,9,8,
	1,0,3,2,5,4,7,6,9,8,
	9,7,8,6,4,5,3,1,2,0,
	0,1,4,3,2,7,6,5,8,9,
	9,7,8,5,6,3,4,1,2,0,
	1,0,7,6,4,5,3,2,9,8,
	0,1,7,6,4,5,3,2,8,9,
	1,0,4,3,2,7,6,5,9,8,
	0,1,4,3,2,7,6,5,8,9,
	1,0,7,4,3,6,5,2,9,8,
	0,1,7,4,3,6,5,2,8,9,
	1,0,7,5,6,3,4,2,9,8,
	0,1,7,5,6,3,4,2,8,9,
	0,1,4,3,2,7,6,5,8,9,
	0,1,5,6,7,2,3,4,8,9,
	0,1,5,6,7,2,3,4,8,9,
	0,1,7,3,5,4,6,2,8,9,
	0,1,7,3,4,5,6,2,8,9,
	0,1,2,6,5,4,3,7,8,9,
	0,2,1,4,3,6,5,8,7,9,
	0,2,1,4,3,6,5,8,7,9,
	7,5,9,3,8,1,6,0,4,2,
	2,1,0,5,6,3,4,9,8,7,
	2,1,0,4,3,6,5,9,8,7,
	4,3,7,1,0,9,8,2,6,5,
	2,1,0,4,3,6,5,9,8,7,
	0,2,1,4,3,6,5,8,7,9,
	2,1,0,4,3,6,5,9,8,7,
	3,2,1,0,4,5,9,8,7,6,
	0,3,2,1,5,4,8,7,6,9,
	4,3,2,1,0,9,8,7,6,5,
	0,4,3,2,1,8,7,6,5,9,
	0,4,3,2,1,8,7,6,5,9,
	9,7,8,6,4,5,3,1,2,0,
	4,3,2,1,0,9,8,7,6,5,
	4,3,2,1,0,9,8,7,6,5,
	0,8,5,6,7,2,3,4,1,9,
	8,9,5,6,7,2,3,4,0,1
};
