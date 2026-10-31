/*
  Persistent adaptive history for the Sprinter scanner.
  Three bounded, integrity-checked microSD journals: adaptive recent snapshots,
  15-minute min/mean/max envelopes, and one updatable record per UTC day.
  Never erases or writes the NVS, OTA partition, or bootloader.
*/
#include <SD.h>
#include <FS.h>
#include <stdlib.h>
#include <stdint.h>
// The existing scanner code mounts SD before starting history. Never autoformat
// external or internal storage: failure is reported, not silently erased.
bool sdCardReady();
uint32_t historyNow();
void monitorPersistedSample(uint8_t module);
static uint32_t hSuccessfulWrites=0,hWriteErrors=0;
static uint32_t hClockAnchorEpoch=0,hClockAnchorMs=0,hClockJumps=0;
static String hClockSource="unverified";
String historyPath(const char *name){
  return String("/t1nlogs")+name;
}
String historyClockJson(){
  uint32_t now=historyNow();
  return "{\"valid\":"+String(now?"true":"false")
    +",\"epoch\":"+String(now)+",\"uptimeMs\":"+String(millis())
    +",\"source\":\""+hClockSource+"\",\"jumps\":"+String(hClockJumps)
    +",\"offlinePowerLossSafe\":false}";
}
void historyClockTick(){
  const uint32_t now=historyNow(),ms=millis();
  if(!now)return;
  if(!hClockAnchorEpoch){hClockAnchorEpoch=now;hClockAnchorMs=ms;
    if(hClockSource=="unverified")hClockSource="system-time (verify sync source)";
  }else if(ms-hClockAnchorMs>=60000UL){
    int64_t expected=(int64_t)hClockAnchorEpoch+(uint32_t)((ms-hClockAnchorMs)/1000UL);
    if(llabs((int64_t)now-expected)>5)hClockJumps++;
    hClockAnchorEpoch=now;hClockAnchorMs=ms;
  }
}
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>

#include "history_record.h"
#include "bounded_json.h"
struct HistoryRing {
  const char *path;
  uint16_t capacity, used=0, next=0;
  uint32_t serial=1;
};
static HistoryRing hRings[3]={
  {"/hist_now.dat",32000},
  {"/hist_15m.dat",32000},
  {"/hist_day.dat",16000}
};
static bool hReady=false;
static String hError;
static uint32_t hLastTick=0,hLastFlush=0,hNextWriteRetryMs=0;

struct HistorySensor {
  uint32_t key=0,quarter=0,day=0,lastWrite=0;
  float lastSaved=0,qMin=0,qMax=0,dMin=0,dMax=0;
  double qSum=0,dSum=0;
  uint32_t qCount=0,dCount=0;
  int16_t dailySlot=-1;
  uint32_t lastSampleMs=0;
  bool sampleSeen=false,recentSet=false,dirty=false;
};
static HistorySensor hSensors[160];
static uint32_t hSensorOverflow=0,hCorruptRecords=0,hReadErrors=0,hMaxReadMs=0,hMaxWriteMs=0,hLatestEpoch=0;
HistorySensor *historySensor(uint32_t key);

uint32_t historyKey(uint8_t module,const char *name){
  uint32_t h=2166136261UL;
  h=(h^module)*16777619UL;h=(h^'/')*16777619UL;
  while(*name)h=(h^(uint8_t)*name++)*16777619UL;
  return h;
}
uint32_t historyNow(){
  time_t t=time(nullptr);
  return t>=1609459200LL&&t<4102444800LL?(uint32_t)t:0;
}
void historyRecover(HistoryRing &r){
  File f=SD.open(historyPath(r.path),"r");
  if(!f){if(SD.exists(historyPath(r.path))){hReadErrors++;hError="Cannot read SD history journal: "+String(r.path);hReady=false;}return;}
  r.used=min((size_t)r.capacity,f.size()/sizeof(HistoryRecord));
  uint32_t newest=0;uint16_t last=0;
  for(uint16_t slot=0;slot<r.used;slot++){
    HistoryRecord record;
    if(f.read((uint8_t*)&record,sizeof(record))!=sizeof(record)){hReadErrors++;hError="SD history recovery read failed";hReady=false;break;}
    if(!historyValid(record)){hCorruptRecords++;continue;}
    hLatestEpoch=max(hLatestEpoch,record.epoch);
    // Recover the latest daily accumulator once, rather than scanning the whole
    // daily journal for every sensor when the first live sample arrives.
    if(&r==&hRings[2]){
      HistorySensor *s=historySensor(record.key);
      if(s&&record.epoch>=s->day){s->day=record.epoch;s->dailySlot=slot;
        s->dMin=record.minimum;s->dMax=record.maximum;
        s->dCount=record.samples;s->dSum=(double)record.average*record.samples;}
    }
    if(record.serial>=newest){
      newest=record.serial;last=slot;
    }
  }
  f.close();
  r.next=r.used<r.capacity?r.used:(uint16_t)((last+1)%r.capacity);
  r.serial=newest+1;if(!r.serial)r.serial=1;
}
bool historyRead(HistoryRing &r,uint16_t slot,HistoryRecord &record){
  File f=SD.open(historyPath(r.path),"r");
  bool ok=f&&f.seek((size_t)slot*sizeof(record),SeekSet)
    &&f.read((uint8_t*)&record,sizeof(record))==sizeof(record);
  if(f)f.close();
  if(!ok){hReadErrors++;hError="Cannot read SD history journal: "+String(r.path);return false;}
  if(!historyValid(record)){hCorruptRecords++;return false;}
  return true;
}
bool historyWrite(HistoryRing &r,uint16_t slot,HistoryRecord &record,bool advance){
  if(!hReady)return false;
  // Bound retries after storage failure; live measurements remain available.
  if(hNextWriteRetryMs&&(int32_t)(millis()-hNextWriteRetryMs)<0)return false;
  const uint32_t started=millis();
  // Seek to the aligned slot even if a prior power loss left a partial tail.
  File f=SD.open(historyPath(r.path),"r+");
  if(!f && slot==0 && r.used==0 && !SD.exists(historyPath(r.path)))
    f=SD.open(historyPath(r.path),"w");
  if(!f){
    hWriteErrors++;
    hError="Cannot open SD history journal: "+String(r.path);
    hNextWriteRetryMs=millis()+30000;
    return false;
  }
  record.checksum=historyChecksum(record);
  bool positioned=f.seek((size_t)slot*sizeof(record),SeekSet);
  size_t written=positioned?f.write((const uint8_t*)&record,sizeof(record)):0;
  f.flush();f.close();
  hMaxWriteMs=max(hMaxWriteMs,(uint32_t)(millis()-started));
  if(written!=sizeof(record)){
    hWriteErrors++;
    hError="SD history write failed: "+String(r.path)+" ("+String(written)+"/"+String(sizeof(record))+" bytes)";
    hNextWriteRetryMs=millis()+30000;
    return false;
  }
  hError="";
  hSuccessfulWrites++;
  hLatestEpoch=max(hLatestEpoch,record.epoch);
  hNextWriteRetryMs=0;
  if(advance){
    if(r.used<r.capacity)r.used++;
    r.next=(slot+1)%r.capacity;
  }
  return true;
}
int historyAppend(int tier,HistoryRecord record){
  HistoryRing &r=hRings[tier];
  record.serial=r.serial++;
  uint16_t slot=r.next;
  return historyWrite(r,slot,record,true)?(int)slot:-1;
}
HistoryRecord historyMake(uint32_t key,uint32_t epoch,float lo,float hi,float avg,uint32_t n){
  HistoryRecord r={};
  r.key=key;r.epoch=epoch;r.minimum=lo;r.maximum=hi;
  r.average=avg;r.samples=n;return r;
}
float historyDelta(const String &unit,float v){
  if(unit=="V")return 0.05f;
  if(unit=="rpm")return 20.0f;
  if(unit=="°C")return 0.5f;
  if(unit=="km/h")return 1.0f;
  if(unit=="bar")return 0.1f;
  if(unit=="hPa"||unit=="mbar")return 10.0f;
  if(unit=="%")return 1.0f;
  return max(0.05f,fabsf(v)*0.02f);
}
HistorySensor *historySensor(uint32_t key){
  for(auto &s:hSensors)if(s.key==key)return &s;
  for(auto &s:hSensors)if(!s.key){s=HistorySensor();s.key=key;return &s;}
  hSensorOverflow++;
  return nullptr;
}
void historySaveDay(HistorySensor &s){
  if(!hReady||!s.dirty||!s.dCount)return;
  HistoryRecord record=historyMake(s.key,s.day,s.dMin,s.dMax,
                                   (float)(s.dSum/s.dCount),s.dCount);
  if(s.dailySlot>=0){
    HistoryRecord prior;const uint32_t readErrorsBefore=hReadErrors;
    if(historyRead(hRings[2],s.dailySlot,prior)
       &&prior.key==s.key&&prior.epoch==s.day){
      record.serial=prior.serial;
      if(historyWrite(hRings[2],s.dailySlot,record,false))s.dirty=false;
      return;
    }
    // A transport read failure must not turn a daily update into a duplicate.
    if(hReadErrors!=readErrorsBefore)return;
    s.dailySlot=-1;
  }
  int slot=historyAppend(2,record);
  if(slot>=0){s.dailySlot=slot;s.dirty=false;}
}
void historyStartDay(HistorySensor &s,uint32_t day){
  s.day=day;s.dailySlot=-1;s.dMin=0;s.dMax=0;s.dSum=0;s.dCount=0;s.dirty=false;
}
void historyCloseQuarter(HistorySensor &s){
  if(!s.qCount||!hReady)return;
  float avg=(float)(s.qSum/s.qCount);
  HistoryRecord r=historyMake(s.key,s.quarter,s.qMin,s.qMax,avg,s.qCount);
  if(historyAppend(1,r)<0)return; // retain pending aggregate on SD failure
  s.qCount=0;s.qSum=0;
}
void historyObserve(const char *id,const ValueState &v){
  if(!hReady||!diagnosticSession||!livePolling||!v.valid||stale(v)||!isfinite(v.value))return;
  // A textual state can retain an old numeric "value"; verify its displayed text.
  char *end=nullptr;double t=strtod(v.text.c_str(),&end);
  if(end==v.text.c_str()||*end!='\0'||!isfinite(t))return;
  uint32_t now=historyNow();if(!now)return;
  const uint32_t sampleAgeSeconds=(uint32_t)(millis()-v.updatedMs)/1000UL;
  if(sampleAgeSeconds>now)return;
  now-=sampleAgeSeconds; // UTC of the received sample, never browser refresh time
  HistorySensor *ptr=historySensor(historyKey((uint8_t)activeModule,id));
  if(!ptr)return;
  HistorySensor &s=*ptr;
  if(s.sampleSeen&&s.lastSampleMs==v.updatedMs)return;
  s.sampleSeen=true;s.lastSampleMs=v.updatedMs;
  float value=(float)v.value;
  uint32_t day=now-now%86400UL,quarter=now-now%900UL;
  if(s.day!=day){
    if(s.day&&s.dCount){historySaveDay(s);if(s.dirty)return;}
    historyStartDay(s,day);
  }
  if(s.quarter!=quarter){
    if(s.quarter){historyCloseQuarter(s);if(s.qCount)return;}
    s.quarter=quarter;s.qCount=0;s.qSum=0;
  }
  if(!s.qCount){s.qMin=s.qMax=value;}else{
    s.qMin=min(s.qMin,value);s.qMax=max(s.qMax,value);
  }
  s.qSum+=value;s.qCount++;
  if(!s.dCount){s.dMin=s.dMax=value;}else{
    s.dMin=min(s.dMin,value);s.dMax=max(s.dMax,value);
  }
  s.dSum+=value;s.dCount++;s.dirty=true;
  // Adaptive events with a 10 s minimum and a 30 min unchanged heartbeat.
  bool changed=!s.recentSet
    ||fabsf(value-s.lastSaved)>=historyDelta(v.unit,s.lastSaved);
  if(!s.recentSet||((changed&&now-s.lastWrite>=10)
                     ||now-s.lastWrite>=1800)){
    HistoryRecord event=historyMake(s.key,now,value,value,value,1);
    if(historyAppend(0,event)>=0){
      s.recentSet=true;s.lastSaved=value;s.lastWrite=now;
      monitorPersistedSample((uint8_t)activeModule);
    }
  }
}
void historyBegin(){
  configTime(0,0,"pool.ntp.org","time.google.com");
  if(!sdCardReady()){hError="microSD not mounted; persistent history unavailable";return;}
  if(!SD.exists("/t1nlogs")&&!SD.mkdir("/t1nlogs")){
    hError="Cannot create microSD /t1nlogs";return;
  }
  hReady=true;
  for(auto &r:hRings)historyRecover(r);
}
void historyTick(){
  historyClockTick();
  if(!hReady)return;
  uint32_t ms=millis();
  if(ms-hLastTick<1000)return;
  hLastTick=ms;
  const uint32_t now=historyNow();
  // Flush completed UTC buckets once even when their module is not active.
  // Keeping keyed accumulators avoids duplicate quarter records on every visit.
  if(now){for(auto &sensor:hSensors)
    if(sensor.key&&sensor.qCount&&now>=sensor.quarter+900UL)historyCloseQuarter(sensor);}
  if(diagnosticSession&&livePolling&&now)historyObserveSensors();
  if(ms-hLastFlush>=900000UL){ // Persist the current day's running summary.
    hLastFlush=ms;
    for(auto &s:hSensors)if(s.key)historySaveDay(s);
  }
}
String historyStatusJson(){
  String s="{\"schemaVersion\":1,\"available\":"+String(hReady?"true":"false")
    +",\"timeReady\":"+String(historyNow()?"true":"false")
    +",\"storage\":\"microSD\",\"capacityBytes\":"
    +String(((uint32_t)hRings[0].capacity+hRings[1].capacity+hRings[2].capacity)*sizeof(HistoryRecord))
    +",\"usedBytes\":"+String(((uint32_t)hRings[0].used+hRings[1].used+hRings[2].used)*sizeof(HistoryRecord))
    +",\"successfulWrites\":"+String(hSuccessfulWrites)+",\"writeErrors\":"+String(hWriteErrors)
    +",\"corruptRecords\":"+String(hCorruptRecords)+",\"readErrors\":"+String(hReadErrors)
    +",\"maxReadMs\":"+String(hMaxReadMs)+",\"maxWriteMs\":"+String(hMaxWriteMs)
    +",\"recording\":{\"minimumChangeSeconds\":10,\"heartbeatSeconds\":1800}"
    +",\"integrity\":\"FNV-1a-32\",\"sensorCapacity\":"+String((unsigned)(sizeof(hSensors)/sizeof(hSensors[0])))
    +",\"sensorOverflow\":"+String(hSensorOverflow)
    +",\"clock\":"+historyClockJson()
    +",\"error\":\""+jsonEscape(hError)+"\",\"tiers\":[";
  for(int i=0;i<3;i++){
    if(i)s+=",";
    s+="{\"name\":\""+String(i==0?"recent":i==1?"15m":"daily")
      +"\",\"records\":"+String(hRings[i].used)
      +",\"capacity\":"+String(hRings[i].capacity)+"}";
  }
  return s+"]}";
}
// One bounded query at a time, advanced between diagnostic transactions.
// Only records successfully persisted to SD enter this cache. The newest 200
// records are returned chronologically; truncation is explicit to the client.
struct HistoryPoint {HistoryRecord record;uint8_t tier;};
static const unsigned HISTORY_QUERY_LIMIT=200;
static HistoryPoint hQuery[HISTORY_QUERY_LIMIT];
struct HistoryQuery {
  bool active=false,ready=false,failed=false,truncated=false;
  uint8_t module=0,tier=0;
  uint16_t slot=0,used[3]={};unsigned count=0;
  uint32_t lastRequest=0,now=0,after=0,key=0,corrupt=0;
  String id,range;File file;
};
static HistoryQuery hQueryState;
void historyQueryTick(){
  HistoryQuery &q=hQueryState;
  if(!q.active)return;
  if(millis()-q.lastRequest>15000UL){if(q.file)q.file.close();q=HistoryQuery();return;}
  if(q.ready||busy||jobConnect)return;
  const uint32_t sliceStarted=millis();
  for(unsigned step=0;step<128;step++){
    if(q.tier>=3){q.ready=true;break;}
    if(q.slot>=q.used[q.tier]){if(q.file)q.file.close();q.tier++;q.slot=0;continue;}
    if(!q.file){q.file=SD.open(historyPath(hRings[q.tier].path),"r");if(!q.file){q.failed=true;break;}}
    HistoryRecord r;
    if(q.file.read((uint8_t*)&r,sizeof(r))!=sizeof(r)){q.failed=true;break;}
    q.slot++;
    if(!historyValid(r)){q.corrupt++;}
    else if(r.key==q.key&&historyInWindow(r,q.tier,q.after,q.now)){
      unsigned pos=0;
      while(pos<q.count&&historyNewer(hQuery[pos].record,hQuery[pos].tier,r,q.tier))pos++;
      if(q.count==HISTORY_QUERY_LIMIT)q.truncated=true;else q.count++;
      if(pos<HISTORY_QUERY_LIMIT){
        for(unsigned j=q.count-1;j>pos;j--)hQuery[j]=hQuery[j-1];
        hQuery[pos]={r,q.tier};
      }
    }
    if(millis()-sliceStarted>=2)break;
  }
  hMaxReadMs=max(hMaxReadMs,(uint32_t)(millis()-sliceStarted));
  if(q.failed){if(q.file)q.file.close();q.ready=true;hReadErrors++;hError="Cannot read microSD history journal";}
}
void historyQueryError(int status,const char *code,const char *message){
  // Fixed error strings keep even low-memory failures valid JSON.
  web.send(status,"application/json",String("{\"ok\":false,\"code\":\"")+code+"\",\"error\":\""+message+"\",\"points\":[]}");
}
void historyDataSend(uint8_t module,const String &id,const String &range){
  web.sendHeader("Cache-Control","no-store");
  if(module>5||!id.length()||id.length()>50){historyQueryError(400,"INVALID_SENSOR","Invalid module or sensor ID");return;}
  if(range!="1h"&&range!="24h"&&range!="7d"&&range!="30d"&&range!="all"){
    historyQueryError(400,"INVALID_RANGE","Choose 1h, 24h, 7d, 30d or all");return;
  }
  if(!hReady){historyQueryError(503,"HISTORY_UNAVAILABLE","microSD history unavailable");return;}
  const uint32_t now=historyNow();
  if(!now&&range!="all"){historyQueryError(503,"CLOCK_INVALID","Clock unknown; choose all retained history");return;}
  HistoryQuery &q=hQueryState;
  if(q.active&&millis()-q.lastRequest>15000UL){if(q.file)q.file.close();q=HistoryQuery();}
  bool same=q.active&&q.module==module&&q.id==id&&q.range==range;
  if(q.active&&!same){web.sendHeader("Retry-After","1");historyQueryError(503,"HISTORY_QUERY_BUSY","Another SD query is in progress");return;}
  if(!q.active){
    q=HistoryQuery();q.active=true;q.module=module;q.id=id;q.range=range;
    q.now=now?now:hLatestEpoch;q.key=historyKey(module,id.c_str());
    uint32_t span=range=="1h"?3600UL:range=="24h"?86400UL:range=="7d"?604800UL:2592000UL;
    q.after=range=="all"?0:(now>span?now-span:0);
    // Tier windows are disjoint: recent covers <=1h, quarter covers
    // >1h through 24h, daily covers >24h. Skip files that cannot match.
    q.used[0]=hRings[0].used;
    q.used[1]=range=="1h"?0:hRings[1].used;
    q.used[2]=(range=="1h"||range=="24h")?0:hRings[2].used;
  }
  q.lastRequest=millis();
  // readRaw() can serve HTTP from inside an ECU response wait. Keep that
  // response short; stream the completed chart only between transactions.
  if(!q.ready||busy||jobConnect){
    web.send(202,"application/json","{\"ok\":false,\"code\":\"HISTORY_LOADING\",\"error\":\"Reading saved SD records\",\"retryAfterMs\":250,\"points\":[]}");return;
  }
  if(q.failed){q.active=false;historyQueryError(503,"HISTORY_READ_FAILED","Cannot read microSD history journal");return;}
  sendScannerJson([&](BoundedJson &out){
  out.raw("{\"ok\":true,\"schemaVersion\":1,\"storage\":\"microSD\",\"module\":");out.integer(module);
  out.raw(",\"id\":");out.string(id.c_str());out.raw(",\"range\":");out.string(range.c_str());
  out.raw(",\"clockValid\":");out.raw(now?"true":"false");
  out.raw(",\"corruptRecordsSkipped\":");out.integer(q.corrupt);out.raw(",\"points\":[");
  for(unsigned i=0;i<q.count;i++){
    const HistoryPoint &p=hQuery[q.count-1-i];const HistoryRecord &r=p.record;
    if(i)out.raw(",");
    out.raw("{\"t\":");out.integer(r.epoch);
    out.raw(",\"min\":");out.number(r.minimum);out.raw(",\"max\":");out.number(r.maximum);
    out.raw(",\"avg\":");out.number(r.average);out.raw(",\"count\":");out.integer(r.samples);
    out.raw(",\"tier\":");out.integer(p.tier);out.raw("}");
  }
  out.raw("],\"count\":");out.integer(q.count);out.raw(",\"truncated\":");out.raw(q.truncated?"true":"false");
  out.raw(",\"recording\":{\"minimumChangeSeconds\":10,\"heartbeatSeconds\":1800}}");
  });q=HistoryQuery();
}
bool historySetClock(uint32_t seconds){
  if(seconds<1609459200UL||seconds>=4102444800UL)return false;
  const uint32_t prior=historyNow();
  struct timeval tv={};tv.tv_sec=seconds;
  bool ok=settimeofday(&tv,nullptr)==0;
  if(ok){if(prior&&llabs((int64_t)seconds-prior)>5)hClockJumps++;hClockSource="browser/manual";hClockAnchorEpoch=seconds;hClockAnchorMs=millis();}
  return ok;
}