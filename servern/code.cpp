#include "Stream.hpp"
#include <vector>
#include <cmath>

namespace BSNCode {

const unsigned char base58[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

const unsigned char debase58[104] = {
	255,0,1,2,3,4,5,6,7,8,255,255,255,255,255,255,
	255,9,10,11,12,13,14,15,16,255,17,18,19,20,21,255,
	22,23,24,25,26,27,28,29,30,31,32,255,255,255,255,255,
	255,33,34,35,36,37,38,39,40,41,42,43,255,44,45,46,
	47,48,49,50,51,52,53,54,55,56,57,255,255,255,255,255
};

void mulBc(unsigned c, unsigned m, std::vector<unsigned short>& v, unsigned short B)
{
	if(0==c && 1==m) return;
	c = v[0] * m + c;
	v[0] = c % B; c /= B;
	for(size_t i=1; 1; ++i) {
		if(i==v.size()) {
			if(!c) break;
			v.push_back(0);
		}
		c += v[i]*m;
		v[i] = c % B;
		c /= B;
	}
}

unsigned divB( unsigned d, std::vector<unsigned short>& v, unsigned short B)
{
	if(d==1)
		return 0;
	unsigned a = 0;
	size_t i = v.size()-1;
	do {
		a = a*B + v[i];
		v[i] = a / d;
		a = a % d;
	} while(i--);
	return a;
}

// base58 encode, decode
//

std::string base58enc(CUnchStream st, size_t pad=1)
{
	std::vector<unsigned short> v (pad,0);
	st.reset(st.base,st.pos());
	//st.setpos(0);
	while(st.left()) {
		mulBc(st.r1(),255,v,58);
	}
	std::string r (v.size(),0);
	for(size_t i=0; i<v.size(); ++i) r[i] = base58[v[i]];
	return r;
}

}/*namespace*/;
