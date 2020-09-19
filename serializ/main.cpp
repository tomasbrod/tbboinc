#include <stdio.h>
#include <string>
#include <tinyxml2.h>
#include "serialize.hpp"
using namespace tinyxml2;
using namespace std;

#define SFIELD(name) s.v( #name, name, sizeof(name) )

struct FILE_INFO {
	char name[256];
	double nbytes;
	int status;
	bool sticky;
	template<class Seralizer> void serialize(Seralizer& s) {
		s.openTag("file_info");
		SFIELD(name);
		SFIELD(nbytes);
		SFIELD(status);
		SFIELD(sticky);
		s.closeTag();
	}
};

struct DOCUMENT {
	char variety[256];
	std::string msg_text;
	int number;
	bool flag1;
	FILE_INFO file_info;
	template<class Seralizer> void serialize(Seralizer& s) {
		s.openTag("document");
		SFIELD(variety);
		SFIELD(msg_text);
		SFIELD(number);
		SFIELD(flag1);
		SFIELD(file_info);
		s.closeTag();
	}
};

int main(void)
{
	DOCUMENT st;
	strcpy(st.variety, "SampleVariaty");
	st.msg_text = "txt";
	st.number=42;
	st.flag1=1;
	strcpy(st.file_info.name, "SampleFileName");
	st.file_info.nbytes=10;
	st.file_info.status=3;
	st.file_info.sticky=1;
	XmlArchiver ar;
	st.serialize(ar);
	puts(ar.pr.CStr());
	XMLDocument doc;
	if(doc.Parse(ar.pr.CStr()))
		return 2;
	XmlUnarchiver un(doc);
	DOCUMENT st2;
	st2.serialize(un);
	printf("variety: %s\n",st.variety);
	printf("file_info.name: %s\n",st.file_info.name);
	//const char* title = doc.FirstChildElement( "PLAY" )->FirstChildElement( "TITLE" )->GetText();
	tinyxml2::XMLDocument doc2;
	doc2.LoadFile( "dream.xml" );
	//doc2.SaveFile(stdout);
	return 1;
}

/*
 * New parser.
 * Parse only one level deep.
 * Do not copy the contents, just cut it up using nulls and put pointers to multimap.
 * parse the sub-element on openTag()
 * simple regular+counter automata parser
 * ? assume a element, cant contain child element of the same tag name -> strstr
 * or count opening/closing tags
 * unescape only strings - not numbers or fragments
 */
