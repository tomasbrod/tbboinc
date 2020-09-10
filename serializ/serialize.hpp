#include <stdint.h>
#include <stddef.h>
#include <string>
#include <cstring>
#include <cassert>
#include <vector>
#include <tinyxml2.h>

struct XmlArchiver {
  tinyxml2::XMLPrinter pr;
  void v ( const char* name, int& value, size_t size = 4 ) {
    pr.OpenElement(name);
    pr.PushText(value);
    pr.CloseElement();
  };
  void v ( const char* name, double& value, size_t size = 0 ) {
    pr.OpenElement(name);
    pr.PushText(value);
    pr.CloseElement();
  };
  void v ( const char* name, uint64_t& value, size_t size = 8  );
  void v ( const char* name, char* value, size_t size ) {
    pr.OpenElement(name);
    pr.PushText(value);
    pr.CloseElement();
  };
  void v ( const char* name, std::string& value, size_t size = 0 ) {
    pr.OpenElement(name);
    pr.PushText(value.c_str());
    pr.CloseElement();
  };
  void v ( const char* name, bool& value, size_t size = 4 ) {
    if(value) {
      pr.OpenElement(name);
      pr.CloseElement();
    }
  };
  template <class T>
  void v ( const char* name, T& value, size_t size = 0 ) {
    //todo: do not ignore char*name
    value.serialize(*this);
  }

  // This is the vector of well-behaved structs inside a containing tag
  template <typename T>
  void v ( const char* name, std::vector<T>& value, size_t size = 0 ) {
    pr.OpenElement(name);
    for(typename std::vector<T>::iterator it = value.begin(); it!=value.end(); ++it) {
      it->serialize(*this);
    }
    pr.CloseElement();
  }

  // This is the vector of well-behaved structs without a containing tag
  template <typename T>
  void v ( std::vector<T>& value ) { //todo
    for(typename std::vector<T>::iterator it = value.begin(); it!=value.end(); ++it) {
      it->serialize(*this);
    }
  }

  void openTag(const char* name) {
    pr.OpenElement(name);
  }
  void closeTag() {
    pr.CloseElement();
  }
  XmlArchiver()
    : pr(NULL, false, 0)
  {
    pr.PushHeader(false, false);
  }
};


struct XmlUnarchiver {
  typedef tinyxml2::XMLNode Node;
  Node *cur;
  void v ( const char* name, char* value, size_t size ) {
    Node* nd1 = cur->FirstChildElement(name);
    printf("FirstChildElement(%s) -> %s\n",name,nd1?nd1->Value():"null");
    //FIXME: if there are tags inside, this will not work
    //FIXME: crashes when tag not found
    Node* nd2 = nd1->FirstChild();
    printf("nd1->FirstChild() -> %s\n",nd2?nd2->Value():"null");
    const char* text = nd2->Value();
    strncpy(value, text, size);
  };
  template <class T> void v ( const char* name, T& value, size_t size = 0 ) {
    //todo: do not ignore char*name
    value.serialize(*this);
  }
  void v ( const char* name, int& value, size_t size = 4 ) {/*stub*/};
  void v ( const char* name, double& value, size_t size = 0 ) {/*stub*/};
  void v ( const char* name, uint64_t& value, size_t size = 8  );
  void v ( const char* name, std::string& value, size_t size ) {/*stub*/};
  void v ( const char* name, bool& value, size_t size = 4 ) {/*stub*/};
  template <typename T> void v ( std::vector<T>& value ) {/*stub*/}
  template <typename T> void v ( const char* name, std::vector<T>& value, size_t size = 0 ) {/*stub*/}


  void openTag(const char* name) {
    Node* nd1 = cur->FirstChildElement(name);
    printf("FirstChildElement(%s) -> %s\n",name,nd1?nd1->Value():"null");
    cur=nd1;
  }
  void closeTag() {
    cur=cur->Parent();
  }
  XmlUnarchiver(tinyxml2::XMLNode& _node)
    : cur(&_node)
  {
  }
};

/* note: Most structs write their own enclosing tag, but some have few
  alternative enclosing tags. How to? Well-behaved structs will be serialzied
  using SFIELD macro. The special structs will be serialized using
  substruct_with_tagname, which will disable one openTag/closeTag call from
  within the nested serialize.
*/
