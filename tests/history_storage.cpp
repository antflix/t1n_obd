// Native regression tests run the production SD history code on a faultable FS.
#include <cassert>
#include <iostream>
#include <cstdlib>
#include "FS.h"
fs::FS SD;
uint32_t ticks=100000;
uint32_t millis(){return ticks;}
unsigned clockRequests=0;
void configTime(int,int,const char*,const char*){clockRequests++;}
bool cardReady=true,busy=false,jobConnect=false,diagnosticSession=true,livePolling=true;
int activeModule=0,persisted=0;
bool sdCardReady(){return cardReady;}
struct ValueState {bool valid=true;double value=1;String text="1",unit="V";uint32_t updatedMs=ticks;};
bool stale(const ValueState&v){return ticks-v.updatedMs>24000;}
void historyObserveSensors(){}
String jsonEscape(const String&s){return s;}
void monitorPersistedSample(uint8_t){persisted++;}
constexpr size_t CONTENT_LENGTH_UNKNOWN=~size_t(0);
struct Web {int code=0;String body;
 void sendHeader(const char*,const char*){}void setContentLength(size_t){}
 void send(int c,const char*,const String&b){code=c;body=b;}
 void sendContent(const char*s,size_t n){body.append(s,n);}void sendContent(const char*){}
}web;
#include "http_json.h"
struct TestResponse {
 std::string wire;
 static uint32_t now(void*){return ticks;}
 static ptrdiff_t send(const char *s,size_t n,void *p){static_cast<TestResponse*>(p)->wire.append(s,n);return n;}
 static void wait(void*){ticks++;}
 static void close(void*){}
};
template <typename Emit> void sendScannerJson(Emit emit){
 TestResponse socket;
 BoundedHttp response({&socket,TestResponse::now,TestResponse::send,TestResponse::wait,TestResponse::close});
 assert(writeHttpJson(response,emit));
 auto split=socket.wire.find("\r\n\r\n");assert(split!=std::string::npos);
 assert(socket.wire.find("Content-Length: "+std::to_string(socket.wire.size()-split-4)+"\r\n")!=std::string::npos);
 web.code=200;web.body=socket.wire.substr(split+4);
}
#include <time.h>
time_t testEpoch=1793300000;
time_t fakeTime(time_t*){return testEpoch;}
#define time fakeTime
#include "36_history.ino"
#undef time

void query(uint8_t module,const String&id,const String&range){
 historyDataSend(module,id,range);
 if(web.code==202){
   for(unsigned i=0;i<10000&&!hQueryState.ready;i++)historyQueryTick();
   historyDataSend(module,id,range);
 }
}
HistoryRecord make(unsigned n,uint32_t epoch=0){
 auto a=historyMake(historyKey(0,"rpm"),epoch?epoch:1760000000+n,n,n,n,1);a.serial=n;return a;
}
void resetRings(){
 SD.files.clear();hRings[0]=HistoryRing{"/hist_now.dat",32000};
 hRings[1]=HistoryRing{"/hist_15m.dat",32000};hRings[2]=HistoryRing{"/hist_day.dat",16000};
 hQueryState=HistoryQuery();
}
int main(){
 // Missing SD is a visible failure; no other filesystem exists in this test.
 cardReady=false;historyBegin();assert(!hReady&&clockRequests==1);query(0,"rpm","24h");
 assert(web.code==503&&web.body.find("HISTORY_UNAVAILABLE")!=String::npos);
 cardReady=true;SD.failMkdir=true;historyBegin();assert(!hReady);SD.failMkdir=false;
 historyBegin();assert(hReady);
 query(0,"rpm","24h");assert(web.code==200&&web.body.find("\"count\":0")!=String::npos);
 HistoryRing r={"/test",2};auto a=make(1),b=make(2),c=make(3);
 assert(historyWrite(r,r.next,a,true));assert(historyWrite(r,r.next,b,true));assert(historyWrite(r,r.next,c,true));
 assert(r.used==2&&r.next==1);HistoryRing restored={"/test",2};historyRecover(restored);assert(restored.next==1&&restored.serial==4);
 auto &bytes=SD.files["/t1nlogs/test"];bytes[sizeof(HistoryRecord)+9]^=0x55;
 historyRecover(restored);assert(hCorruptRecords==1);HistoryRecord read;
 assert(historyRead(restored,0,read)&&read.serial==3);assert(!historyRead(restored,1,read));
 // A partially written tail is overwritten at the record boundary, never appended misaligned.
 HistoryRing tail={"/tail",3};a=make(1);assert(historyWrite(tail,0,a,true));
 SD.files["/t1nlogs/tail"].insert(SD.files["/t1nlogs/tail"].end(),11,0x88);
 HistoryRing recovered={"/tail",3};historyRecover(recovered);assert(recovered.used==1);
 b=make(2);assert(historyWrite(recovered,1,b,true));assert(historyRead(recovered,1,read)&&read.serial==2);
 SD.failOpen=true;ticks+=30001;c=make(3);auto errors=hWriteErrors;
 assert(!historyWrite(recovered,2,c,true));assert(hWriteErrors==errors+1);SD.failOpen=false;
 assert(!historyWrite(recovered,2,c,true));ticks+=30001;SD.shortWrite=true;
 assert(!historyWrite(recovered,2,c,true));SD.shortWrite=false;ticks+=30001;
 assert(historyWrite(recovered,2,c,true));assert(hError.empty());
 // Each received ECU sample contributes exactly once, independently of UI requests.
 ValueState value;historyObserve("battery",value);auto count=hSensors[0].qCount;
 historyObserve("battery",value);assert(hSensors[0].qCount==count);
 ticks++;value.updatedMs=ticks;historyObserve("battery",value);assert(hSensors[0].qCount==count+1);
 // Failed aggregate writes retain the exact pending count and sum for retry.
 HistorySensor aggregate;aggregate.key=historyKey(0,"aggregate");
 aggregate.quarter=(uint32_t)testEpoch-1000;aggregate.qMin=1;aggregate.qMax=3;
 aggregate.qCount=2;aggregate.qSum=4;
 SD.failOpen=true;historyCloseQuarter(aggregate);assert(aggregate.qCount==2&&aggregate.qSum==4);
 SD.failOpen=false;ticks+=30001;historyCloseQuarter(aggregate);assert(!aggregate.qCount);
 // Long uptime must not interpret an inactive retry deadline (zero) as future.
 ticks=0x90000000UL;a=make(4);assert(historyWrite(recovered,0,a,false));ticks=200000;
 // Recovery loads each key's latest daily accumulator once for continued aggregation.
 HistorySensor *daily=historySensor(historyKey(0,"day-test"));
 daily->day=(uint32_t)testEpoch-(uint32_t)testEpoch%86400;daily->dMin=1;daily->dMax=3;
 daily->dCount=2;daily->dSum=4;daily->dirty=true;historySaveDay(*daily);
 assert(!daily->dirty);
 daily->dirty=true;auto dailyUsed=hRings[2].used;SD.failRead=true;historySaveDay(*daily);
 assert(daily->dirty&&hRings[2].used==dailyUsed);SD.failRead=false;historySaveDay(*daily);assert(!daily->dirty);
 uint32_t dayKey=daily->key;*daily=HistorySensor();
 historyRecover(hRings[2]);daily=historySensor(dayKey);assert(daily->dCount==2&&daily->dSum==4);
 // Exhaustion is bounded and counted rather than overwriting another sensor.
 for(auto &sensor:hSensors)if(!sensor.key)sensor.key=123;
 auto overflows=hSensorOverflow;assert(historySensor(987654)==nullptr&&hSensorOverflow==overflows+1);
 for(auto &sensor:hSensors)sensor=HistorySensor();
 // Missing or unreadable nonempty journals produce explicit errors, never empty success.
 hRings[0].used=1;SD.files.erase("/t1nlogs/hist_now.dat");query(0,"rpm","24h");
 assert(web.code==503&&web.body.find("HISTORY_READ_FAILED")!=String::npos);
 resetRings();a=make(1,testEpoch);assert(historyWrite(hRings[0],0,a,true));
 SD.failRead=true;query(0,"rpm","24h");assert(web.code==503);SD.failRead=false;
 // One request never scans SD synchronously, and transaction ownership prevents work.
 historyDataSend(0,"rpm","24h");assert(web.code==202);busy=true;
 historyQueryTick();assert(!hQueryState.ready);busy=false;
 ticks+=15001;historyQueryTick();assert(!hQueryState.active);
 // A completed result must also stay queued inside a diagnostic response wait.
 historyDataSend(0,"rpm","24h");while(!hQueryState.ready)historyQueryTick();
 busy=true;historyDataSend(0,"rpm","24h");assert(web.code==202&&hQueryState.ready);
 busy=false;historyDataSend(0,"rpm","24h");assert(web.code==200&&!hQueryState.active);
 // Bounded newest selection survives out-of-order timestamps and physical rollover.
 resetRings();hRings[0].capacity=240;
 for(unsigned j=1;j<=500;j++){auto rec=make(j,(uint32_t)testEpoch-500+j);assert(historyWrite(hRings[0],hRings[0].next,rec,true));}
 query(0,"rpm","1h");assert(web.code==200);
 assert(web.body.find("\"count\":200")!=String::npos&&web.body.find("\"truncated\":true")!=String::npos);
 auto first=web.body.find("\"t\":"+String((uint32_t)testEpoch-199));
 auto last=web.body.find("\"t\":"+String((uint32_t)testEpoch));assert(first<last);
 assert(web.body.size()<30000&&web.body.find("\"schemaVersion\":1")!=String::npos);
 // The same SD query returns identical records after a simulated browser reload.
 String savedResponse=web.body;auto savedWrites=hSuccessfulWrites;
 query(0,"rpm","1h");assert(web.body==savedResponse&&hSuccessfulWrites==savedWrites);
 // Active ECU/module changes cannot redirect a pending saved-history query.
 historyDataSend(0,"rpm","1h");activeModule=2;
 while(!hQueryState.ready){historyQueryTick();}
 historyDataSend(0,"rpm","1h");
 assert(web.body==savedResponse);activeModule=0;
 // Abandoned completed results expire as well, releasing the single query slot.
 historyDataSend(0,"rpm","1h");while(!hQueryState.ready)historyQueryTick();
 ticks+=15001;historyQueryTick();assert(!hQueryState.active);
 query(0,"rpm","1h");assert(web.code==200);
 String status=historyStatusJson();assert(status.find("\"available\":true")!=String::npos);
 assert(status.find("\"enabled\"")==String::npos);
 // Tier windows are disjoint, never include future timestamps or fabricate gaps.
 a=make(1);assert(!historyInWindow(a,0,0,a.epoch-1));
 assert(historyInWindow(a,0,0,a.epoch+3600));assert(!historyInWindow(a,1,0,a.epoch+3600));
 assert(historyInWindow(a,1,0,a.epoch+3601));
 resetRings();
 auto put=[](int tier,uint32_t age){auto rec=make(age,(uint32_t)testEpoch-age);assert(historyWrite(hRings[tier],hRings[tier].next,rec,true));};
 put(0,10);put(0,1200);put(1,4000);put(1,80000);put(2,90000);put(2,700000);put(2,2700000);
 query(0,"rpm","1h");assert(web.body.find("\"count\":2")!=String::npos);
 query(0,"rpm","24h");assert(web.body.find("\"count\":4")!=String::npos);
 query(0,"rpm","7d");assert(web.body.find("\"count\":5")!=String::npos);
 query(0,"rpm","30d");assert(web.body.find("\"count\":6")!=String::npos);
 query(0,"rpm","all");assert(web.body.find("\"count\":7")!=String::npos);
 query(1,"rpm","24h");assert(web.body.find("\"count\":0")!=String::npos);
 testEpoch=0;query(0,"rpm","1h");assert(web.code==503&&web.body.find("CLOCK_INVALID")!=String::npos);
 query(0,"rpm","all");assert(web.code==200&&web.body.find("\"clockValid\":false")!=String::npos);
 query(0,"rpm","invalid");assert(web.code==400);query(9,"rpm","all");assert(web.code==400);
 // Query-level tier skipping preserves exact hour/day boundaries and avoids
 // opening unrelated journals, even if those irrelevant files are unreadable.
 resetRings();testEpoch=1793300000;
 put(0,3600);put(0,3601);
 put(1,3600);put(1,3601);put(1,86400);
 put(2,86400);put(2,86401);
 historyDataSend(0,"rpm","1h");assert(web.code==202);
 assert(hQueryState.used[0]==2&&hQueryState.used[1]==0&&hQueryState.used[2]==0);
 query(0,"rpm","1h");assert(web.code==200&&web.body.find("\"count\":1")!=String::npos);
 historyDataSend(0,"rpm","24h");assert(web.code==202);
 assert(hQueryState.used[0]==2&&hQueryState.used[1]==3&&hQueryState.used[2]==0);
 query(0,"rpm","24h");assert(web.code==200&&web.body.find("\"count\":3")!=String::npos);
 query(0,"rpm","7d");assert(web.code==200&&web.body.find("\"count\":4")!=String::npos);
 SD.files.erase("/t1nlogs/hist_day.dat");
 query(0,"rpm","24h");assert(web.code==200&&web.body.find("\"count\":3")!=String::npos);
 SD.files.erase("/t1nlogs/hist_15m.dat");
 query(0,"rpm","1h");assert(web.code==200&&web.body.find("\"count\":1")!=String::npos);
 query(0,"rpm","7d");assert(web.code==503&&web.body.find("HISTORY_READ_FAILED")!=String::npos);
 assert(!historySetClock(0));assert(!historySetClock(0xffffffff));
 std::cout<<"SD-only history: absent storage, failed reads/writes, backoff, checksum corruption, interrupted tails, rollover, dedup, bounded queries, ranges/gaps and invalid clock PASS\n";
}
