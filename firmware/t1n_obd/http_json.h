#pragma once
#include "bounded_json.h"
#include "bounded_http.h"
// Callers capture one immutable snapshot before invoking this two-pass writer.
// Counting and emitting use the same serializer; no response-sized allocation.
template <typename Emit> bool writeHttpJson(BoundedHttp &response,Emit emit){
  size_t length=0;
  {BoundedJson counter([](const char*,size_t n,void *p){*static_cast<size_t*>(p)+=n;},&length);
  emit(counter);counter.flush();}
  if(!response.begin(length))return false;
  {BoundedJson output([](const char *s,size_t n,void *p){static_cast<BoundedHttp*>(p)->write(s,n);},&response);
  emit(output);output.flush();}
  return response.finish();
}
