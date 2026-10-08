/*
  Persistent adaptive history for the Sprinter scanner.
  Three bounded, CRC-checked SPIFFS journals: adaptive recent snapshots,
  15-minute min/mean/max envelopes, and one updatable record per UTC day.
  Never erases or writes the NVS, OTA partition, or bootloader.
*/
#include <SPIFFS.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <stdlib.h>

struct HistoryRecord {
  uint32_t serial, epoch, key;
  float minimum, maximum, average;
  uint32_t samples, checksum;
};
static_assert(sizeof(HistoryRecord)==32,"History record size changed");

struct HistoryRing {
  const char *path;
  uint16_t capacity, used=0, next=0;
  uint32_t serial=1;
};
static HistoryRing hRings[3]={
  {"/hist_now.dat",768}, // 24 KiB
  {"/hist_15m.dat",768}, // 24 KiB
  {"/hist_day.dat",1200} // 37.5 KiB
};
static bool hReady=false;
static String hError;
static uint32_t hLastTick=0,hLastFlush=0;
static int hLastModule=-1;

struct HistorySensor {
  uint32_t key=0,quarter=0,day=0,lastWrite=0,lastQuarter=0;
  float lastSaved=0,qMin=0,qMax=0,dMin=0,dMax=0;
  double qSum=0,dSum=0;
  uint32_t qCount=0,dCount=0;
  int16_t dailySlot=-1;
  bool recentSet=false,dirty=false;
};
static HistorySensor hSensors[100];

uint32_t historyKey(uint8_t module,const char *name){
  uint32_t h=2166136261UL;
  h=(h^module)*16777619UL;h=(h^'/')*16777619UL;
  while(*name)h=(h^(uint8_t)*name++)*16777619UL;
  return h;
}
uint32_t historyChecksum(const HistoryRecord &r){
  uint32_t h=2166136261UL;
  const uint8_t *p=(const uint8_t*)&r;
  for(size_t n=0;n<sizeof(r)-4;n++)h=(h^p[n])*16777619UL;
  return h;
}
bool historyValid(const HistoryRecord &r){
  return r.serial&&r.serial!=0xffffffffUL&&r.epoch>=1609459200UL&&r.key
    &&r.samples&&isfinite(r.minimum)&&isfinite(r.maximum)&&isfinite(r.average)
    &&r.minimum<=r.maximum&&r.checksum==historyChecksum(r);
}
uint32_t historyNow(){
  time_t t=time(nullptr);
  return t>=1609459200LL&&t<4102444800LL?(uint32_t)t:0;
}
void historyRecover(HistoryRing &r){
  File f=SPIFFS.open(r.path,"r");if(!f)return;
  r.used=min((size_t)r.capacity,f.size()/sizeof(HistoryRecord));
  uint32_t newest=0;uint16_t last=0;
  for(uint16_t slot=0;slot<r.used;slot++){
    HistoryRecord record;
    if(f.read((uint8_t*)&record,sizeof(record))!=sizeof(record))break;
    if(historyValid(record)&&record.serial>=newest){
      newest=record.serial;last=slot;
    }
  }
  f.close();
  r.next=r.used<r.capacity?r.used:(uint16_t)((last+1)%r.capacity);
  r.serial=newest+1;if(!r.serial)r.serial=1;
}
bool historyRead(HistoryRing &r,uint16_t slot,HistoryRecord &record){
  File f=SPIFFS.open(r.path,"r");if(!f)return false;
  bool ok=f.seek((size_t)slot*sizeof(record),SeekSet)
    &&f.read((uint8_t*)&record,sizeof(record))==sizeof(record);
  f.close();return ok&&historyValid(record);
}
bool historyWrite(HistoryRing &r,uint16_t slot,HistoryRecord &record,bool advance){
  if(!hReady)return false;
  File f=SPIFFS.open(r.path,"r+");
  if(!f)f=SPIFFS.open(r.path,"w+");
  if(!f){hError="Cannot open flash history journal";return false;}
  record.checksum=historyChecksum(record);
  bool ok=f.seek((size_t)slot*sizeof(record),SeekSet)
    &&f.write((const uint8_t*)&record,sizeof(record))==sizeof(record);
  f.flush();f.close();
  if(!ok){hError="Flash history write failed";return false;}
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
  File f=SPIFFS.open(r.path,"r");
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
  float variation=s.qMax-s.qMin;
  // Stable parameters do not need an envelope every 15 minutes.
  if(variation>max(0.05f,fabsf(avg)*0.015f)
     ||!s.lastQuarter||s.quarter-s.lastQuarter>=4*3600UL){
    HistoryRecord r=historyMake(s.key,s.quarter,s.qMin,s.qMax,avg,s.qCount);
    if(historyAppend(1,r)>=0)s.lastQuarter=s.quarter;
  }
  s.qCount=0;s.qSum=0;
}
void historyCloseModule(){
  for(auto &s:hSensors){
    if(!s.key)continue;
    historyCloseQuarter(s);historySaveDay(s);
    s=HistorySensor();
  }
}
void historyObserve(const char *id,const ValueState &v){
  if(!hReady||!diagnosticSession||!livePolling||!v.valid||stale(v)||!isfinite(v.value))return;
  // A textual state can retain an old numeric "value"; verify its displayed text.
  char *end=nullptr;double t=strtod(v.text.c_str(),&end);
  if(end==v.text.c_str()||*end!='\0'||!isfinite(t))return;
  uint32_t now=historyNow();if(!now)return;
  HistorySensor *ptr=historySensor(historyKey((uint8_t)activeModule,id));
  if(!ptr)return;
  HistorySensor &s=*ptr;
  float value=(float)v.value;
  uint32_t day=now-now%86400UL,quarter=now-now%900UL;
  if(s.day!=day){
    if(s.day&&s.dCount)historySaveDay(s);
    historyStartDay(s,day);
  }
  if(s.quarter!=quarter){
    if(s.quarter)historyCloseQuarter(s);
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
    }
  }
}
void historyBegin(){
  // First use formats ONLY the dedicated SPIFFS data partition, not NVS/OTA.
  hReady=SPIFFS.begin(true);
  if(!hReady){hError="Unable to mount SPIFFS";return;}
  if(SPIFFS.totalBytes()<110000){
    hReady=false;hError="Flash history partition smaller than expected";
    SPIFFS.end();return;
  }
  for(auto &r:hRings)historyRecover(r);
  // NTP runs asynchronously. The browser also supplies UTC for offline use.
  configTime(0,0,"pool.ntp.org","time.google.com");
}
void historyTick(){
  if(!hReady)return;
  uint32_t ms=millis();
  if(ms-hLastTick<1000)return;
  hLastTick=ms;
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
    +",\"capacityBytes\":"+String(hReady?SPIFFS.totalBytes():0)
    +",\"usedBytes\":"+String(hReady?SPIFFS.usedBytes():0)
    +",\"error\":\""+jsonEscape(hError)+"\",\"tiers\":[";
  for(int i=0;i<3;i++){
    if(i)s+=",";
    s+="{\"name\":\""+String(i==0?"recent":i==1?"15m":"daily")
      +"\",\"records\":"+String(hRings[i].used)
      +",\"capacity\":"+String(hRings[i].capacity)+"}";
  }
  return s+"]}";
}
// Send bounded time series from each tier, oldest to newest. Older resolutions
// are included only outside newer tiers' windows to avoid double plotting.
String historyDataJson(uint8_t module,const String &id,const String &range){
  if(!hReady)return "{\"error\":\"History storage unavailable\",\"points\":[]}";
  if(id.length()==0||id.length()>50||module>2)return "{\"error\":\"Invalid sensor\",\"points\":[]}";
  uint32_t now=historyNow();
  if(!now)return "{\"error\":\"Clock not synchronized\",\"points\":[]}";
  uint32_t span=range=="1h"?3600UL:range=="24h"?86400UL:
    range=="7d"?604800UL:range=="30d"?2592000UL:315360000UL;
  uint32_t after=now>span?now-span:0,key=historyKey(module,id);
  // Limit the response so a busy web client cannot exhaust ESP32 heap.
  String s="{\"ok\":true,\"points\":[";
  bool first=true;int emitted=0;
  for(int tier=2;tier>=0;tier--){
    if(range=="1h"&&tier!=0)continue;
    if(range=="24h"&&tier==2)continue;
    HistoryRing &ring=hRings[tier];
    File f=SPIFFS.open(ring.path,"r");if(!f)continue;
    for(uint16_t j=0;j<ring.used;j++){
      uint16_t slot=ring.used==ring.capacity?(ring.next+j)%ring.capacity:j;
      HistoryRecord r;
      if(!f.seek((size_t)slot*sizeof(r),SeekSet)
        ||f.read((uint8_t*)&r,sizeof(r))!=sizeof(r))continue;
      if(!historyValid(r)||r.key!=key||r.epoch<after||r.epoch>now+120)continue;
      uint32_t age=now-r.epoch;
      if(tier==0 && age>3600UL)continue;
      if(tier==1 && (age<=3600UL || age>7*86400UL))continue;
      if(tier==2 && age<=7*86400UL && range!="30d"&&range!="all")continue;
      if(tier==2 && age<=86400UL)continue;
      if(emitted>=350)break;
      if(!first)s+=",";
      first=false;
      s+="{\"t\":"+String(r.epoch)
        +",\"min\":"+String(r.minimum,4)
        +",\"max\":"+String(r.maximum,4)
        +",\"avg\":"+String(r.average,4)
        +",\"count\":"+String(r.samples)
        +",\"tier\":"+String(tier)+"}";
      emitted++;
    }
    f.close();
  }
  return s+"],\"count\":"+String(emitted)+"}";
}
bool historySetClock(uint32_t seconds){
  if(seconds<1760000000UL||seconds>4102444800UL)return false;
  struct timeval tv={};tv.tv_sec=seconds;
  return settimeofday(&tv,nullptr)==0;
}