#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <cstring>
#include <cstdio>
using std::min;using std::max;
class String:public std::string {
 public:
 using std::string::string;
 String()=default;String(const std::string& s):std::string(s){}
 bool reserve(size_t n){std::string::reserve(n);return true;}
 void remove(size_t at,size_t n){erase(at,n);}
 int indexOf(char c,size_t at){auto i=find(c,at);return i==npos?-1:(int)i;}
 void concat(const char*p,size_t n){append(p,n);}
 String(unsigned long n):std::string(std::to_string(n)){}
 String(unsigned n):std::string(std::to_string(n)){}
 String(int n):std::string(std::to_string(n)){}
 String(float n,int d):std::string(std::to_string(n)){(void)d;}
};
inline String operator+(const String&a,const String&b){return String(std::string(a)+std::string(b));}
inline String operator+(const char*a,const String&b){return String(std::string(a)+std::string(b));}
inline String operator+(const String&a,const char*b){return String(std::string(a)+std::string(b));}
constexpr int SeekSet=0;
namespace fs {class FS;}
class File {
 public:
 std::vector<uint8_t>* data=nullptr;size_t pos=0;bool* shortWrite=nullptr;
 explicit operator bool()const{return data;}
 size_t size()const{return data?data->size():0;}
 bool seek(size_t p,int=0){if(!data||p>data->size())return false;pos=p;return true;}
 size_t read(uint8_t*out,size_t n){if(!data)return 0;n=min(n,data->size()-pos);memcpy(out,data->data()+pos,n);pos+=n;return n;}
 size_t write(const uint8_t*in,size_t n){if(!data)return 0;if(shortWrite&&*shortWrite)n/=2;data->resize(max(data->size(),pos+n));memcpy(data->data()+pos,in,n);pos+=n;return n;}
 size_t available()const{return data?data->size()-pos:0;}
 int read(){uint8_t c;return read(&c,1)==1?c:-1;}
 void flush(){}void close(){data=nullptr;}
};
namespace fs {class FS {
 public:
 std::map<std::string,std::vector<uint8_t>> files;
 bool failOpen=false,shortWrite=false;
 File open(const String&name,const char*mode){
  if(failOpen)return {};
  auto it=files.find(name);
  if(it==files.end()&&mode[0]!='w'&&mode[0]!='a')return {};
  if(mode[0]=='w')files[name].clear();
  auto &v=files[name];File f;f.data=&v;f.shortWrite=&shortWrite;f.pos=mode[0]=='a'?v.size():0;return f;
 }
 bool exists(const String&p){return files.count(p);}
 bool mkdir(const String&){return true;}
 uint32_t totalBytes(){return 100000000;}uint32_t usedBytes(){return 0;}
};}
