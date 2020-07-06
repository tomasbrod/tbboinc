#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include "exact_cover_u.h"

using namespace std;

const int timeout = 5 * CLOCKS_PER_SEC;

list<kvadrat> baza_dlk;
map<kvadrat_m,list<kvadrat_m>> baza_mar;
char* input = "input.txt";
bool mute, name, order;
int por, raz, raz_kvm;

void pars_arg(int argc, char* argv[]);
int init();
void vyvod(const string& str);

int main(int argc, char* argv[]){
	setlocale(LC_CTYPE, "rus");
	if(argc > 1) pars_arg(argc, argv);
	if(int ret = init()){
		if(!mute){
			cout << "Для выхода нажмите любую клавишу: ";
			system("pause > nul");
			cout << endl;
		}
		return ret;
	}
	clock_t t0 = clock(), tb, te;
	tb = t0;
	int count = 0;
	for(list<kvadrat>::iterator q = baza_dlk.begin(); q != baza_dlk.end(); q++){
		exact_cover::search_mates(*q);
		count++;
		if((te = clock()) - tb > timeout){
			cout << "Проверено ДЛК: " << count << " найдено ОДЛК: " << baza_mar.size() << " время: "
				<< (te - t0) / CLOCKS_PER_SEC << " сек\n";
			tb = te;
		}
	}
	vyvod(name ? string("m_") + input : string("output.txt"));
	if(!mute){
		cout << "Найдено ОДЛК: " << baza_mar.size() << endl;
		cout << "Время работы: " << double(clock() - t0) / CLOCKS_PER_SEC << " сек\n\n";
		cout << "Для выхода нажмите любую клавишу: ";
		system("pause > nul");
		cout << endl;
	}
}

void pars_arg(int argc, char* argv[]){
	for(int i = 1, c = 0; i < argc && c < 3; i++){
		if(isdigit(argv[i][0])){
			if(!order){
				por = atoi(argv[i]);
				order = true;
				c++;
			}
		}
		else if(argv[i][0] == '/'){
			if(!mute && (argv[i][1] == 'm' || argv[i][1] == 'M')){
				mute = true;
				c++;
			}
		}
		else if(!name){
			input = argv[i];
			name = true;
			c++;
		}
	}
}

inline void zapis(char ch, kvadrat& kv, int& count){
	static bool rem;
	if(rem){
		if(ch == ']') rem = false;
		return;
	}
	if(ch == '['){
		rem = true;
		return;
	}
	int t;
	if(isdigit(ch) && (t = ch - '0') < por ||
	   isupper(ch) && (t = ch - 'A' + 10) < por ||
	   islower(ch) && (t = ch - 'a' + 10) < por){
		kv[count++] = t;
		if(count == raz){
			if(is_dlk(kv)) baza_dlk.push_back(kv);
			count = 0;
		}
	}
}

int init(){
	const int raz_buf = 0x1000;
	if(!order || por < 4 || por > 36) while(true){
		cout << "Введите порядок: ";
		cin >> por;
		if(por >= 4 && por <= 36) break;
		else cout << "Число должно принадлежать интервалу [4..36]\n";
	}
	system("cls");
	cout << "Проверка ДЛК" << por << " на марьяжность (ОДЛК)\n\n";
	raz = por * por;
	raz_kvm = (raz << 1) + por;
	exact_cover();
	ifstream fin(input, ios::binary);
	if(!fin){cerr << "Нет файла " << input << endl; return 1;}
	kvadrat tempk(raz);
	int count = 0;
	char bufer[raz_buf];
	while(fin.read(bufer, raz_buf)) for(int i = 0; i < raz_buf; i++) zapis(bufer[i], tempk, count);
	if(fin.eof()) for(int i = 0; i < fin.gcount(); i++) zapis(bufer[i], tempk, count);
	if(baza_dlk.empty()){cerr << "Нет ДЛК в файле " << input << endl; return 2;}
	cout << "Введено ДЛК:  " << baza_dlk.size() << endl;
	return 0;
}

void vyvod(const string& str){
	if(baza_mar.empty()) return;
	ofstream fout(str, ios::binary);
	int count;
	for(iter_mar q = baza_mar.begin(); q != baza_mar.end(); q++){
		ostringstream ss;
		ss << "[DLK(" << q->second.size() << ")]\r\n";
		fout.write(ss.str().c_str(), ss.str().size());
		fout.write((char*)q->first.data(), raz_kvm);
		count = 0;
		for(list<kvadrat_m>::iterator w = q->second.begin(); w != q->second.end(); w++){
			ostringstream ss;
			ss << "[mate#" << ++count << "]\r\n";
			fout.write(ss.str().c_str(), ss.str().size());
			fout.write((char*)w->data(), raz_kvm);
		}
		fout.write("\r\n", 2);
	}
}
