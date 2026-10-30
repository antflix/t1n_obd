// Exercise the actual firmware journal/recovery/query functions on a faultable FS.
#include <cassert>
#include <iostream>
#include <cstdlib>
#include "FS.h"
fs::FS SD,SPIFFS;
uint32_t ticks=100000;
uint32_t millis(){return ticks;}
void delay(unsigned){}
void configTime(int,int,const char*,const char*){}
bool cardReady=true,busy=false,jobConnect=false,diagnosticSession=true,livePolling=true;
int activeModule=0,persisted=0;
bool sdCardReady(){return cardReady;}
struct ValueState {bool valid=true;double value=1;String text="1",unit="V";uint32_t updatedMs=1;};
bool stale(const ValueState&){return false;}
String sensorsJson(bool){return {};}
String jsonEscape(const String&s){return s;}
void monitorPersistedSample(uint8_t){persisted++;}
constexpr size_t CONTENT_LENGTH_UNKNOWN=~size_t(0);
struct Web {int code=0;String body;
 void sendHeader(const char*,const char*){}void setContentLength(size_t){}
 void send(int c,const char*,const String&b){code=c;body=b;}
 void sendContent(const char*s,size_t n){body.append(s,n);}void sendContent(const char*){}
}web;
void sensorHttpSink(const char*s,size_t n,void*){web.sendContent(s,n);}
#include <time.h>
time_t testEpoch=1793300000;
time_t fakeTime(time_t*){return testEpoch;}
#define time fakeTime
#include "36_history.ino"
#undef time

void query(uint8_t module,const String&id,const String&range,uint32_t before=0,uint32_t serial=0,uint8_t tier=0){
 historyDataSend(module,id,range,before,serial,tier);
 if(web.code==202){for(unsigned i=0;i<10000&&!hQueryState.ready;i++)historyQueryTick();historyDataSend(module,id,range,before,serial,tier);}
}

int main(){
 historyBegin();assert(hReady);
 HistoryRing r={"/test",2};
 auto make=[](unsigned n){auto a=historyMake(historyKey(0,"rpm"),1760000000+n,n,n,n,1);a.serial=n;return a;};
 auto a=make(1),b=make(2),c=make(3);
 assert(historyWrite(r,r.next,a,true));assert(historyWrite(r,r.next,b,true));assert(historyWrite(r,r.next,c,true));
 assert(r.used==2&&r.next==1);HistoryRing restored={"/test",2};historyRecover(restored);assert(restored.next==1&&restored.serial==4);
 auto &bytes=SD.files["/t1nlogs/test"];bytes[sizeof(HistoryRecord)+9]^=0x55;
 historyRecover(restored);assert(hCorruptRecords==1);HistoryRecord read;assert(historyRead(restored,0,read)&&read.serial==3);assert(!historyRead(restored,1,read));
 // Interrupted append leaves a partial tail: next write must overwrite aligned EOF.
 HistoryRing tail={"/tail",3};a=make(1);assert(historyWrite(tail,0,a,true));
 SD.files["/t1nlogs/tail"].insert(SD.files["/t1nlogs/tail"].end(),11,0x88);
 HistoryRing recovered={"/tail",3};historyRecover(recovered);assert(recovered.used==1);b=make(2);assert(historyWrite(recovered,1,b,true));assert(historyRead(recovered,1,read)&&read.serial==2);
 SD.failOpen=true;ticks+=30001;c=make(3);auto errors=hWriteErrors;assert(!historyWrite(recovered,2,c,true));assert(hWriteErrors==errors+1);SD.failOpen=false;
 // Backoff prevents hammering storage after an open failure.
 assert(!historyWrite(recovered,2,c,true));ticks+=30001;SD.shortWrite=true;assert(!historyWrite(recovered,2,c,true));SD.shortWrite=false;ticks+=30001;assert(historyWrite(recovered,2,c,true));assert(hError.empty());
 // Repeated history ticks must not multiply a single real source sample.
 ValueState value;historyObserve("battery",value);auto count=hSensors[0].qCount;historyObserve("battery",value);assert(hSensors[0].qCount==count);value.updatedMs++;historyObserve("battery",value);assert(hSensors[0].qCount==count+1);
 // A missing nonempty journal is an error, not empty history.
 hRings[0].used=1;SD.files.erase("/t1nlogs/hist_now.dat");query(0,"rpm","24h");assert(web.code==503&&web.body.find("HISTORY_READ_FAILED")!=String::npos);
 busy=true;query(0,"rpm","24h");assert(web.code==503&&web.body.find("DIAGNOSTIC_BUSY")!=String::npos);busy=false;
 hReady=false;query(0,"rpm","24h");assert(web.code==503&&web.body.find("HISTORY_UNAVAILABLE")!=String::npos);
 // Timestamp ordering survives physical ring rollover and gaps; tiers don't overlap.
 a=make(1);assert(!historyInWindow(a,0,0,a.epoch-1));assert(historyInWindow(a,0,0,a.epoch+3600));assert(!historyInWindow(a,1,0,a.epoch+3600));assert(historyInWindow(a,1,0,a.epoch+3601));
 hReady=true;hRings[0]=HistoryRing{"/hist_now.dat",32000};hRings[1].used=hRings[2].used=0;
 for(unsigned j=1;j<=500;j++){auto rec=historyMake(historyKey(0,"rpm"),(uint32_t)testEpoch-500+j,j,j,j,1);rec.serial=j;assert(historyWrite(hRings[0],hRings[0].next,rec,true));}
 query(0,"rpm","1h");assert(web.code==200);assert(web.body.find("\"count\":200")!=String::npos);assert(web.body.find("\"more\":true")!=String::npos);assert(web.body.find("\"before\":"+String((uint32_t)testEpoch-199))!=String::npos);
 query(0,"rpm","1h",(uint32_t)testEpoch-199,301,0);assert(web.code==200);assert(web.body.find("\"before\":"+String((uint32_t)testEpoch-399))!=String::npos);
 query(0,"rpm","1h",(uint32_t)testEpoch-399,101,0);assert(web.body.find("\"count\":100")!=String::npos);assert(web.body.find("\"more\":false")!=String::npos);
 testEpoch=0;query(0,"rpm","1h");assert(web.code==503&&web.body.find("CLOCK_INVALID")!=String::npos);query(0,"rpm","all");assert(web.code==200&&web.body.find("\"clockValid\":false")!=String::npos);
 assert(!historySetClock(0));assert(!historySetClock(0xffffffff));
 std::cout<<"Pagination, invalid clock, all-history without current UTC PASS\n";
 std::cout<<"Journal rollover, corruption, interrupted tail, failed/short writes, retry, sample dedup and query errors PASS\n";
}
