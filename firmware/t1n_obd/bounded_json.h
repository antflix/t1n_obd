#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

// No heap allocation. The sink receives at most 256 bytes per call, including
// when a sensor label contains quotes, control bytes, or non-ASCII UTF-8.
class BoundedJson {
 public:
  using Sink = void (*)(const char*, size_t, void*);
  BoundedJson(Sink sink, void* context=nullptr): sink_(sink), context_(context) {}
  void flush() { if(used_) { sink_(buffer_,used_,context_); used_=0; } }
  void raw(const char* s) { while(*s) character(*s++); }
  void character(char c) { buffer_[used_++]=c; if(used_==sizeof(buffer_))flush(); }
  void string(const char* s) {
    character('"');
    for(const unsigned char* p=(const unsigned char*)s;*p;p++) {
      if(*p=='"'||*p=='\\') { character('\\');character(*p); }
      else if(*p<32) { char b[7];snprintf(b,sizeof(b),"\\u%04x",*p);raw(b); }
      else character(*p);
    }
    character('"');
  }
  void integer(uint32_t value) { char b[16];snprintf(b,sizeof(b),"%lu",(unsigned long)value);raw(b); }
  void number(double value) {
    if(!isfinite(value)){raw("null");return;}
    char b[40];snprintf(b,sizeof(b),"%.8g",value);raw(b);
  }
 private:
  Sink sink_;void* context_;char buffer_[256];size_t used_=0;
};
