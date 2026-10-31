#include <cassert>
#include <iostream>
#include <string>
#include <algorithm>
#include "http_json.h"

struct Socket {
  uint32_t now=1000,sendCost=0,closeCost=0;
  size_t maxSend=65536,calls=0,closed=0,blockAfter=~size_t(0);
  size_t temporaryStallAfter=~size_t(0);
  uint32_t temporaryStallUntil=0;
  bool fatal=false;
  std::string wire;
  static uint32_t clock(void *p){return ((Socket*)p)->now;}
  static ptrdiff_t send(const char *data,size_t length,void *p){
    auto &s=*(Socket*)p;s.calls++;s.now+=s.sendCost;
    if(s.fatal)return -1;
    if(s.now<s.temporaryStallUntil){
      if(s.wire.size()>=s.temporaryStallAfter)return 0;
      length=std::min(length,s.temporaryStallAfter-s.wire.size());
    }
    if(s.wire.size()>=s.blockAfter)return 0;
    const size_t n=std::min({length,s.maxSend,s.blockAfter-s.wire.size()});
    s.wire.append(data,n);return (ptrdiff_t)n;
  }
  static void wait(void *p){((Socket*)p)->now++;}
  static void stop(void *p){auto &s=*(Socket*)p;s.closed++;s.now+=s.closeCost;}
  BoundedHttp::Operations operations(){return {this,clock,send,wait,stop};}
  std::string body()const {const auto pos=wire.find("\r\n\r\n");assert(pos!=std::string::npos);return wire.substr(pos+4);}
};

int main(){
  // The same serializer determines exact length and sends >8KB without allocation.
  Socket json;json.maxSend=7;BoundedHttp jsonResponse(json.operations());
  assert(writeHttpJson(jsonResponse,[](BoundedJson &out){
    out.raw("{\"items\":[");for(unsigned i=0;i<500;i++){if(i)out.raw(",");out.raw("{\"id\":");out.integer(i);out.raw(",\"label\":");out.string("quoted\"\n\t °C");out.raw("}");}out.raw("]}");
  }));
  assert(json.body().size()>8192);
  assert(json.wire.find("Content-Length: "+std::to_string(json.body().size())+"\r\n")!=std::string::npos);
  Socket packed;BoundedHttp packedResponse(packed.operations());
  assert(writeHttpJson(packedResponse,[](BoundedJson &out){out.raw("{\"value\":");out.string(std::string(12000,'x').c_str());out.raw("}");}));
  assert(packed.calls<=packed.body().size()/1436+3); // Header + full MSS chunks + tail.
  // A selected history pair can exceed 32 KB; its encoder is still streamed
  // through the same fixed MSS buffer and one shared response deadline.
  Socket pair;BoundedHttp pairResponse(pair.operations());
  auto emitPair=[](BoundedJson &out){
    out.raw("{\"series\":[");
    for(unsigned series=0;series<2;series++){
      if(series)out.raw(",");
      out.raw("{\"points\":[");
      for(unsigned i=0;i<200;i++){
        if(i)out.raw(",");
        out.raw("{\"t\":1793300000,\"min\":-123456789.12345,\"avg\":123456789.12345,\"max\":987654321.12345,\"count\":4294967295,\"tier\":2}");
      }
      out.raw("]}");
    }
    out.raw("]}");
  };
  assert(writeHttpJson(pairResponse,emitPair));assert(pair.body().size()>40000);
  assert(pair.wire.find("Content-Length: "+std::to_string(pair.body().size())+"\r\n")!=std::string::npos);
  assert(pair.calls<=pair.body().size()/1436+3);
  // Temporary socket backpressure can clear before the absolute deadline;
  // the real serializer must then complete the exact same header and body.
  const size_t pairHeaderBytes=pair.wire.size()-pair.body().size();
  Socket delayedPair;delayedPair.temporaryStallAfter=pairHeaderBytes+4308;
  delayedPair.temporaryStallUntil=delayedPair.now+240;
  BoundedHttp delayedPairResponse(delayedPair.operations());
  assert(writeHttpJson(delayedPairResponse,emitPair));
  assert(delayedPair.wire==pair.wire&&delayedPairResponse.durationMs()==240);
  assert(delayedPairResponse.error()==BoundedHttp::Error::None&&delayedPair.closed==1);
  // An otherwise identical temporary delay beyond 250 ms is explicitly cut
  // off, even though the socket would become writable again shortly after.
  Socket latePair;latePair.temporaryStallAfter=pairHeaderBytes+4308;
  latePair.temporaryStallUntil=latePair.now+260;
  BoundedHttp latePairResponse(latePair.operations());
  assert(!writeHttpJson(latePairResponse,emitPair));
  assert(latePairResponse.error()==BoundedHttp::Error::Timeout);
  assert(latePairResponse.durationMs()==250&&latePair.closed==1);
  assert(latePair.body().size()==4308&&latePair.wire==pair.wire.substr(0,latePair.wire.size()));
  const auto lateCalls=latePair.calls;latePair.now=latePair.temporaryStallUntil;
  assert(!latePairResponse.write("x",1)&&!latePairResponse.finish());
  assert(latePair.calls==lateCalls&&latePair.closed==1);
  Socket stalledPair;stalledPair.blockAfter=18000;BoundedHttp stalledPairResponse(stalledPair.operations());
  assert(!writeHttpJson(stalledPairResponse,emitPair));
  assert(stalledPairResponse.durationMs()==250&&stalledPair.closed==1);
  Socket closeMeasured;closeMeasured.closeCost=7;BoundedHttp closedResponse(closeMeasured.operations());
  assert(closedResponse.begin(0)&&closedResponse.finish()&&closedResponse.durationMs()==7);
  // Short socket writes are completed without changing Content-Length or JSON.
  Socket partial;partial.maxSend=3;BoundedHttp p(partial.operations());
  assert(p.begin(7));assert(p.write("{\"x\":",5));assert(p.write("1}",2));assert(p.finish());
  assert(partial.wire.find("Content-Length: 7\r\n")!=std::string::npos);
  assert(partial.body()=="{\"x\":1}"&&p.bodyBytes()==7&&p.bytesSent()==partial.wire.size()&&partial.closed==1);
  // A connected peer with a zero receive window must not strand the loop.
  Socket blocked;blocked.blockAfter=0;BoundedHttp b(blocked.operations());
  assert(!b.begin(5));assert(b.error()==BoundedHttp::Error::Timeout&&b.durationMs()==250&&blocked.closed==1);
  const auto attempts=blocked.calls;
  assert(!b.write("hello",5)&&!b.finish()&&!b.begin(5));
  assert(blocked.calls==attempts&&blocked.closed==1);
  // Progress cannot reset the response deadline, even across separate chunks.
  Socket slow;BoundedHttp s(slow.operations());assert(s.begin(500));
  slow.maxSend=1;slow.sendCost=1;
  assert(s.write(std::string(100,'a').data(),100));assert(s.write(std::string(100,'b').data(),100));
  assert(!s.write(std::string(100,'c').data(),100));
  assert(s.durationMs()==250&&s.error()==BoundedHttp::Error::Timeout&&s.bodyBytes()==250&&slow.closed==1);
  // An abort midway through the body stops all subsequent serialization writes.
  Socket aborted;BoundedHttp a(aborted.operations());assert(a.begin(7));
  assert(a.write("abc",3));aborted.fatal=true;assert(!a.write("defg",4));
  const auto abortedCalls=aborted.calls;
  assert(!a.write("defg",4)&&!a.finish()&&aborted.calls==abortedCalls);
  assert(a.error()==BoundedHttp::Error::Io&&a.bodyBytes()==3&&aborted.closed==1);
  // A full header followed by a stalled body has the same single deadline.
  Socket bodyStall;BoundedHttp bs(bodyStall.operations());assert(bs.begin(10));
  bodyStall.blockAfter=bodyStall.wire.size()+4;
  assert(!bs.write("0123456789",10)&&bs.bodyBytes()==4&&bs.durationMs()==250&&bodyStall.closed==1);
  // Never advertise a successful complete response when serialization is short.
  Socket shortBody;BoundedHttp sb(shortBody.operations());assert(sb.begin(8));assert(sb.write("short",5));
  assert(!sb.finish()&&sb.error()==BoundedHttp::Error::Length&&shortBody.closed==1);
  Socket longBody;BoundedHttp lb(longBody.operations());assert(lb.begin(2));assert(!lb.write("long",4));
  assert(lb.error()==BoundedHttp::Error::Length&&lb.bodyBytes()==0&&longBody.closed==1);
  // Unsigned deadline arithmetic survives the millis() rollover.
  Socket rollover;rollover.now=0xfffffff0UL;rollover.blockAfter=0;BoundedHttp r(rollover.operations(),20);
  assert(!r.begin(1)&&r.durationMs()==20&&rollover.closed==1);
  // A caller cannot accidentally extend the maximum response budget.
  Socket capped;capped.blockAfter=0;BoundedHttp c(capped.operations(),10000);
  assert(!c.begin(1)&&c.durationMs()==250);
  Socket empty;BoundedHttp e(empty.operations());assert(e.begin(0)&&e.finish()&&empty.body().empty());
  std::cout<<"Bounded HTTP partial writes, temporary backpressure, stalled peer, slow progress, abort, exact length and rollover PASS\n";
}
