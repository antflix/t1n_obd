#pragma once
#include "FS.h"
class Preferences {
 std::string ns;
 public:
 inline static std::map<std::string,std::vector<uint8_t>> store;
 inline static bool fail=false;
 bool begin(const char*s,bool){ns=s;return !fail;}
 void end(){}
 size_t putBytes(const char*k,const void*p,size_t n){if(fail)return 0;auto b=(const uint8_t*)p;store[ns+":"+k]={b,b+n};return n;}
 size_t getBytesLength(const char*k){return store[ns+":"+k].size();}
 size_t getBytes(const char*k,void*p,size_t n){auto&v=store[ns+":"+k];n=min(n,v.size());memcpy(p,v.data(),n);return n;}
 size_t putString(const char*k,const String&s){return putBytes(k,s.c_str(),s.size()+1);}
 String getString(const char*k,const char*d){auto&v=store[ns+":"+k];return v.empty()?String(d):String((const char*)v.data());}
};
