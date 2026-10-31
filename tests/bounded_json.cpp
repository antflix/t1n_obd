#include <cassert>
#include <iostream>
#include <string>
#include "bounded_json.h"
static std::string result;
void sink(const char* s,size_t n,void*){assert(n<=1436);result.append(s,n);}
int main(){
 BoundedJson json(sink);
 json.raw("{\"items\":[");
 for(unsigned i=0;i<500;i++){
  if(i)json.raw(",");
  json.raw("{\"id\":");json.integer(i);
  json.raw(",\"label\":");json.string("quoted\" \\ line\n tab\t CR\r \001 °C");
  json.raw(",\"value\":");json.number(i*.25);json.raw("}");
 }
 json.raw("],\"invalid\":");json.number(NAN);json.raw("}");json.flush();
 assert(result.size()>8192);std::cout<<result;
}
