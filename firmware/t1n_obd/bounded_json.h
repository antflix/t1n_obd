#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

// No heap allocation. The sink receives at most 1436 bytes per call, including
// when a sensor label contains quotes, control bytes, or non-ASCII UTF-8.
// One configured TCP MSS avoids exhausting lwIP packet slots with tiny writes.
class BoundedJson {
 public:
  struct Workspace { char bytes[1436]; };
  using Sink = void (*)(const char*, size_t, void*);
  BoundedJson(Workspace &workspace,Sink sink,void* context=nullptr)
      : workspace_(workspace),sink_(sink),context_(context) {}
  void flush() { if(used_) { sink_(workspace_.bytes,used_,context_); used_=0; } }
  void raw(const char* s) { while(*s) character(*s++); }
  void character(char c) { workspace_.bytes[used_++]=c; if(used_==sizeof(workspace_.bytes))flush(); }
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
  Workspace &workspace_;Sink sink_;void* context_;size_t used_=0;
};
