#pragma once
#include "bounded_json.h"
#include "bounded_http.h"
// Callers capture one immutable snapshot before invoking this two-pass writer.
// Counting and emitting use the same serializer; no response-sized allocation.
// The caller owns one workspace, reused by these sequential passes.
template <typename Emit> __attribute__((noinline)) bool writeHttpJson(BoundedHttp &response,BoundedJson::Workspace &workspace,Emit emit){
  size_t length=0;
  {BoundedJson counter(workspace,[](const char*,size_t n,void *p){*static_cast<size_t*>(p)+=n;},&length);
  emit(counter);counter.flush();}
  if(!response.begin(length))return false;
  {BoundedJson output(workspace,[](const char *s,size_t n,void *p){static_cast<BoundedHttp*>(p)->write(s,n);},&response);
  emit(output);output.flush();}
  return response.finish();
}
