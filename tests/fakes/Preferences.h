#pragma once
#include "FS.h"

// Process-local NVS fake shared by independently opened Preferences handles.
class Preferences {
  std::string space;
 public:
  inline static std::map<std::string,bool> booleans;
  inline static std::map<std::string,String> strings;
  bool begin(const char *name,bool=false){space=name;return true;}
  void end(){}
  bool getBool(const char *key,bool fallback=false){auto it=booleans.find(space+"/"+key);return it==booleans.end()?fallback:it->second;}
  String getString(const char *key,const char *fallback=""){auto it=strings.find(space+"/"+key);return it==strings.end()?String(fallback):it->second;}
  size_t putBool(const char *key,bool value){booleans[space+"/"+key]=value;return 1;}
  size_t putString(const char *key,const String &value){strings[space+"/"+key]=value;return value.length();}
};
