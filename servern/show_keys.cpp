#include <stdio.h>
#include <string>
#include "parse2.hpp"
#include "build/cofig.hpp"
#include "build/cofig.cpp"
#include <sodium/crypto_hash_sha256.h>
using std::string;

const string c_peer_key = ":peer:";
const string c_plugin_key = ":plug:";
const string c_upload_key = ":upld:";

string to_hex(const CBuffer& data) {
    unsigned int i;
    string out;
    out.resize(data.length()*2);
    const char hex[] = "0123456789abcdef";

    for (i=0; i<data.length(); i++) {
        out[i*2+0] = hex[data.base[i]/16];
        out[i*2+1] = hex[data.base[i]%16];
    }

    return out;
}

int main(void) {

	t_config config;
	{
		CFileStream fs3;
		fs3.openRead("config.xml");
		XML_PARSER2 xp (&fs3);
		xp.get_tag(); // open the root tag, todo check error code
		config.parse(xp);
	}

	printf("Master key         : %s\n", config.server_keys.master.c_str());
	for( const auto& peer : config.peers ) {
		string m = config.server_keys.master + c_peer_key + peer.name;
		CStUnStream<crypto_hash_sha256_BYTES> h;
		crypto_hash_sha256( (unsigned char*)h.base, (unsigned char*)m.c_str(), m.length() );
		printf("Peer %12s  : %s\n", peer.name.c_str(), to_hex(h).c_str());
	}
	for( const auto& sub : config.subs ) {
		string m = config.server_keys.master + c_plugin_key + sub.name;
		CStUnStream<crypto_hash_sha256_BYTES> h;
		crypto_hash_sha256( (unsigned char*)h.base, (unsigned char*)m.c_str(), m.length() );
		printf("Plugin %12s: %s\n", sub.name.c_str(), to_hex(h).c_str());
	}
	{
		string m = config.server_keys.master + c_upload_key;
		CStUnStream<crypto_hash_sha256_BYTES> h;
		crypto_hash_sha256( (unsigned char*)h.base, (unsigned char*)m.c_str(), m.length() );
		printf("Upload key         : %s\n", to_hex(h).c_str());
	}

	return 0;
}
