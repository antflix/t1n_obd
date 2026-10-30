/*
  Persistent adaptive history for the Sprinter scanner.
  Three bounded, integrity-checked microSD journals: adaptive recent snapshots,
  15-minute min/mean/max envelopes, and one updatable record per UTC day.
  Never erases or writes the NVS, OTA partition, or bootloader.
*/
#include <SPIFFS.h>
#include <SD.h>
#include <FS.h>
#include <stdlib.h>
#include <stdint.h>
// The existing scanner code mounts SD before starting history. Never autoformat
// external or internal storage: failure is reported, not silently erased.
bool sdCardReady();
uint32_t historyNow();
void monitorPersistedSample(uint8_t module);
static fs::FS *hFs=nullptr;
static bool hUsingSD=false;
static uint32_t hVerifiedWrites=0,hWriteErrors=0;
static uint32_t hClockAnchorEpoch=0,hClockAnchorMs=0,hClockJumps=0;
static String hClockSource="unverified";
String historyPath(const char *name){
  return hUsingSD ? String("/t1nlogs")+name : String(name);
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
  {"/hist_now.dat",640}, // 20 KiB
  {"/hist_15m.dat",640}, // 20 KiB
  {"/hist_day.dat",1024} // 32 KiB
};
static bool hReady=false;
static String hError;
static uint32_t hLastTick=0,hLastFlush=0,hNextWriteRetryMs=0;
static int hLastModule=-1;

struct HistorySensor {
  uint32_t key=0,quarter=0,day=0,lastWrite=0,lastQuarter=0;
  float lastSaved=0,qMin=0,qMax=0,dMin=0,dMax=0;
  double qSum=0,dSum=0;
  uint32_t qCount=0,dCount=0;
  int16_t dailySlot=-1;
  uint32_t lastSampleMs=0;
  bool sampleSeen=false,recentSet=false,dirty=false;
};
static HistorySensor hSensors[160];
static uint32_t hSensorOverflow=0,hCorruptRecords=0,hReadErrors=0,hMaxReadMs=0,hMaxWriteMs=0;

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
  File f=hFs->open(historyPath(r.path),"r");if(!f)return;
  r.used=min((size_t)r.capacity,f.size()/sizeof(HistoryRecord));
  uint32_t newest=0;uint16_t last=0;
  for(uint16_t slot=0;slot<r.used;slot++){
    HistoryRecord record;
    if(f.read((uint8_t*)&record,sizeof(record))!=sizeof(record))break;
    if(!historyValid(record)){hCorruptRecords++;continue;}
    if(record.serial>=newest){
      newest=record.serial;last=slot;
    }
  }
  f.close();
  r.next=r.used<r.capacity?r.used:(uint16_t)((last+1)%r.capacity);
  r.serial=newest+1;if(!r.serial)r.serial=1;
}
bool historyRead(HistoryRing &r,uint16_t slot,HistoryRecord &record){
  File f=hFs->open(historyPath(r.path),"r");if(!f)return false;
  bool ok=f.seek((size_t)slot*sizeof(record),SeekSet)
    &&f.read((uint8_t*)&record,sizeof(record))==sizeof(record);
  f.close();return ok&&historyValid(record);
}
bool historyWrite(HistoryRing &r,uint16_t slot,HistoryRecord &record,bool advance){
  if(!hReady)return false;
  // A failed journal previously retried every second for every PID, starving K-line polling.
  // Back off on flash errors; keep RAM live data available.
  if((int32_t)(millis()-hNextWriteRetryMs)<0)return false;
  // SPIFFS append creates a new journal and extends it without seeking beyond EOF.
  // Existing/full ring slots use r+ for in-place replacement.
  const uint32_t started=millis();
  // Seek to the aligned slot even if a prior power loss left a partial tail.
  File f=hFs->open(historyPath(r.path),"r+");
  if(!f && slot==0 && r.used==0)f=hFs->open(historyPath(r.path),"w");
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
  hVerifiedWrites++;
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
    HistoryRecord prior;
    if(historyRead(hRings[2],s.dailySlot,prior)
       &&prior.key==s.key&&prior.epoch==s.day){
      record.serial=prior.serial;
      if(historyWrite(hRings[2],s.dailySlot,record,false))s.dirty=false;
      return;
    }
    s.dailySlot=-1;
  }
  int slot=historyAppend(2,record);
  if(slot>=0){s.dailySlot=slot;s.dirty=false;}
}
void historyStartDay(HistorySensor &s,uint32_t day){
  s.day=day;s.dailySlot=-1;s.dMin=0;s.dMax=0;s.dSum=0;s.dCount=0;s.dirty=false;
  HistoryRing &r=hRings[2];uint32_t newest=0;
  File f=hFs->open(historyPath(r.path),"r");
  if(!f)return;
  for(uint16_t slot=0;slot<r.used;slot++){
    HistoryRecord v;
    if(f.read((uint8_t*)&v,sizeof(v))!=sizeof(v))break;
    if(historyValid(v)&&v.key==s.key&&v.epoch==day&&v.serial>=newest){
      newest=v.serial;s.dailySlot=slot;
      s.dMin=v.minimum;s.dMax=v.maximum;
      s.dCount=v.samples;s.dSum=(double)v.average*v.samples;
    }
  }
  f.close();
}
void historyCloseQuarter(HistorySensor &s){
  if(!s.qCount||!hReady)return;
  float avg=(float)(s.qSum/s.qCount);
  HistoryRecord r=historyMake(s.key,s.quarter,s.qMin,s.qMax,avg,s.qCount);
  if(historyAppend(1,r)<0)return; // retain pending aggregate on SD failure
  s.lastQuarter=s.quarter;
  s.qCount=0;s.qSum=0;
}
void historyCloseModule(){
  for(auto &s:hSensors){
    if(!s.key)continue;
    historyCloseQuarter(s);historySaveDay(s);
    if(!s.qCount&&!s.dirty)s=HistorySensor();
  }
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
  // SD is preferred. Never format media holding existing user files.
  hUsingSD=sdCardReady();
  if(hUsingSD){
    if(!SD.exists("/t1nlogs") && !SD.mkdir("/t1nlogs")){
      hError="Cannot create SD /t1nlogs";hUsingSD=false;
    }else{
      hFs=&SD;hReady=true;
      hRings[0].capacity=32000;hRings[1].capacity=32000;hRings[2].capacity=16000;
    }
  }
  if(!hReady){
    hError = "microSD history unavailable: "+(hError.length()?hError:String("SD not mounted"));
    return;
  }
  for(auto &r:hRings)historyRecover(r);
  // Use NTP when reachable. Offline power cycles require an external RTC for
  // authoritative wall time; until synchronized, do not invent timestamps.
}
void historyTick(){
  historyClockTick();
  if(!hReady)return;
  uint32_t ms=millis();
  if(ms-hLastTick<1000)return;
  hLastTick=ms;
  historyClockTick();
  if(hLastModule!=(int)activeModule){
    if(hLastModule>=0)historyCloseModule();
    hLastModule=(int)activeModule;
  }
  if(diagnosticSession&&livePolling&&historyNow())sensorsJson(true);
  if(ms-hLastFlush>=900000UL){ // Persist the current day's running summary.
    hLastFlush=ms;
    for(auto &s:hSensors)if(s.key)historySaveDay(s);
  }
}
String historyStatusJson(){
  String s="{\"enabled\":"+String(hReady?"true":"false")
    +",\"timeReady\":"+String(historyNow()?"true":"false")
    +",\"storage\":\""+String(!hReady?"unavailable":hUsingSD?"microSD":"internal SPIFFS")+"\""
    +",\"capacityBytes\":"+String(hReady?(hUsingSD?
      ((uint32_t)hRings[0].capacity+hRings[1].capacity+hRings[2].capacity)*sizeof(HistoryRecord):
      (uint32_t)SPIFFS.totalBytes()):0)
    +",\"usedBytes\":"+String(hReady?(hUsingSD?
      ((uint32_t)hRings[0].used+hRings[1].used+hRings[2].used)*sizeof(HistoryRecord):
      (uint32_t)SPIFFS.usedBytes()):0)
    +",\"successfulWrites\":"+String(hVerifiedWrites)+",\"writeErrors\":"+String(hWriteErrors)
    +",\"corruptRecords\":"+String(hCorruptRecords)+",\"readErrors\":"+String(hReadErrors)
    +",\"maxReadMs\":"+String(hMaxReadMs)+",\"maxWriteMs\":"+String(hMaxWriteMs)
    +",\"changedMinimumSeconds\":10,\"unchangedHeartbeatSeconds\":1800,\"integrity\":\"FNV-1a-32\""
    +",\"sensorCapacity\":"+String((unsigned)(sizeof(hSensors)/sizeof(hSensors[0])))
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
// Query scratch space is static and bounded; HTTP is single-threaded. Expensive
// SD reads are rejected while a K-line transaction is in progress.
struct HistoryPoint {HistoryRecord record;uint8_t tier;};
static HistoryPoint hQuery[200];
struct HistoryQuery {
  bool active=false,ready=false,failed=false,more=false;
  uint8_t module=0,tier=0,beforeTier=0;
  uint16_t slot=0,used[3]={};unsigned count=0;
  uint32_t started=0,completed=0,now=0,after=0,key=0,before=0,serial=0,corrupt=0;
  String id,range;File file;
};
static HistoryQuery hQueryState;
void historyQueryTick(){
  HistoryQuery &q=hQueryState;if(!q.active||q.ready||busy||jobConnect)return;
  const uint32_t sliceStarted=millis();
  for(unsigned step=0;step<128;step++){
    if(q.tier>=3){q.ready=true;q.completed=millis();break;}
    if(q.slot>=q.used[q.tier]){if(q.file)q.file.close();q.tier++;q.slot=0;continue;}
    if(!q.file){q.file=hFs->open(historyPath(hRings[q.tier].path),"r");if(!q.file){q.failed=true;break;}}
    HistoryRecord r;
    if(q.file.read((uint8_t*)&r,sizeof(r))!=sizeof(r)){q.failed=true;break;}
    q.slot++;
    if(!historyValid(r)){q.corrupt++;}
    else if(r.key==q.key&&r.epoch>=q.after&&(!q.now||historyInWindow(r,q.tier,q.after,q.now))){
      HistoryRecord cursor={};cursor.epoch=q.before;cursor.serial=q.serial;
      if(!q.before||historyNewer(cursor,q.beforeTier,r,q.tier)){
        unsigned pos=0;while(pos<q.count&&historyNewer(hQuery[pos].record,hQuery[pos].tier,r,q.tier))pos++;
        if(q.count==200){q.more=true;}else q.count++;
        if(pos<200){for(unsigned j=q.count-1;j>pos;j--)hQuery[j]=hQuery[j-1];hQuery[pos]={r,q.tier};}
      }
    }
    if(millis()-sliceStarted>=2)break;
  }
  hMaxReadMs=max(hMaxReadMs,(uint32_t)(millis()-sliceStarted));
  if(q.failed){if(q.file)q.file.close();q.ready=true;q.completed=millis();hReadErrors++;}
}
void historyDataSend(uint8_t module,const String &id,const String &range,
                     uint32_t before=0,uint32_t serial=0,uint8_t beforeTier=0){
  web.sendHeader("Cache-Control","no-store");
  if(!hReady){web.send(503,"application/json","{\"ok\":false,\"code\":\"HISTORY_UNAVAILABLE\",\"error\":\"microSD history unavailable\",\"points\":[]}");return;}
  uint32_t now=historyNow();
  if(!now&&range!="all"){web.send(503,"application/json","{\"ok\":false,\"code\":\"CLOCK_INVALID\",\"error\":\"Clock unknown; choose all retained history\",\"points\":[]}");return;}
  if(busy||jobConnect){web.sendHeader("Retry-After","1");web.send(503,"application/json","{\"ok\":false,\"code\":\"DIAGNOSTIC_BUSY\",\"error\":\"History read deferred during diagnostic transaction\",\"points\":[]}");return;}
  HistoryQuery &q=hQueryState;
  bool same=q.active&&q.module==module&&q.id==id&&q.range==range&&q.before==before&&q.serial==serial&&q.beforeTier==beforeTier;
  if(q.active&&!same&&((q.ready&&millis()-q.completed>10000)||millis()-q.started>120000)){
    if(q.file)q.file.close();
    q=HistoryQuery();
  }
  if(q.active&&!same){web.send(503,"application/json","{\"ok\":false,\"code\":\"HISTORY_QUERY_BUSY\",\"error\":\"Another bounded SD query is in progress\",\"points\":[]}");return;}
  if(!q.active){
    q=HistoryQuery();q.active=true;q.module=module;q.id=id;q.range=range;q.before=before;q.serial=serial;q.beforeTier=beforeTier;
    q.started=millis();q.now=now;q.key=historyKey(module,id.c_str());
    uint32_t span=range=="1h"?3600UL:range=="24h"?86400UL:range=="7d"?604800UL:2592000UL;
    q.after=range=="all"?0:(now>span?now-span:0);
    for(unsigned i=0;i<3;i++)q.used[i]=hRings[i].used;
  }
  if(!q.ready){web.send(202,"application/json","{\"ok\":false,\"code\":\"HISTORY_PENDING\",\"error\":\"Reading saved SD records between diagnostic transactions\",\"points\":[]}");return;}
  if(q.failed){q.active=false;web.send(503,"application/json","{\"ok\":false,\"code\":\"HISTORY_READ_FAILED\",\"error\":\"Cannot read SD history journal\",\"points\":[]}");return;}
  const unsigned count=q.count;const bool more=q.more;const uint32_t corrupt=q.corrupt;
  web.setContentLength(CONTENT_LENGTH_UNKNOWN);web.send(200,"application/json","");
  BoundedJson out(sensorHttpSink);
  out.raw("{\"ok\":true,\"source\":\"microSD\",\"module\":");out.integer(module);
  out.raw(",\"id\":");out.string(id.c_str());out.raw(",\"clockValid\":");out.raw(now?"true":"false");
  out.raw(",\"corruptRecordsSkipped\":");out.integer(corrupt);
  out.raw(",\"points\":[");
  for(unsigned i=0;i<count;i++){
    const HistoryPoint &p=hQuery[count-1-i];const HistoryRecord &r=p.record;
    if(i)out.raw(",");
    out.raw("{\"t\":");out.integer(r.epoch);
    out.raw(",\"min\":");out.number(r.minimum);out.raw(",\"max\":");out.number(r.maximum);
    out.raw(",\"avg\":");out.number(r.average);out.raw(",\"count\":");out.integer(r.samples);
    out.raw(",\"tier\":");out.integer(p.tier);out.raw(",\"serial\":");out.integer(r.serial);out.raw("}");
  }
  out.raw("],\"count\":");out.integer(count);out.raw(",\"more\":");out.raw(more?"true":"false");
  if(more&&count){const HistoryPoint &p=hQuery[count-1];out.raw(",\"next\":{\"before\":");out.integer(p.record.epoch);out.raw(",\"serial\":");out.integer(p.record.serial);out.raw(",\"tier\":");out.integer(p.tier);out.raw("}");}
  out.raw("}");out.flush();web.sendContent("");q.active=false;
}
bool historySetClock(uint32_t seconds){
  if(seconds<1760000000UL||seconds>4102444800UL)return false;
  const uint32_t prior=historyNow();
  struct timeval tv={};tv.tv_sec=seconds;
  bool ok=settimeofday(&tv,nullptr)==0;
  if(ok){if(prior&&llabs((int64_t)seconds-prior)>5)hClockJumps++;hClockSource="browser/manual";hClockAnchorEpoch=seconds;hClockAnchorMs=millis();}
  return ok;
}