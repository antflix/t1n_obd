// Read-only autonomous logger. Four physical K-line channels share ONE UART:
// sessions are time-sliced; engine is intentionally prioritized.
// No actuator commands, DTC clearing or unverified DTC services are issued.
#include <Preferences.h>
#include <SD.h>
#include <time.h>
#include "monitor_dtc_diff.h"
extern uint32_t lastDtcScanMs; // defined in 90_setup.ino

struct MonModule {
  uint32_t connects=0,failures=0,recentWrites=0,lastConnectionMs=0;
  uint32_t lastDtcEpoch=0,lastDtcUptimeMs=0;
  bool dtcBaseline=false;
  uint8_t dtcCount=0;
  uint16_t codes[32]={};
  uint8_t statuses[32]={};
};
static MonModule monModules[6];
static bool monEnabled=true,monOwnSession=false;
static uint32_t monStartMs=0,monRetryMs=0,monHoldUntil=0;
static uint32_t monEgsDue=0,monAbsDue=0,monHzrDue=0;
static ModuleId monTarget=MOD_ENGINE;
static uint32_t monDtcEvents=0,monLastClockLogMs=0;
static String monLastError="";

bool monitorManualHold(){
  return monHoldUntil && (int32_t)(monHoldUntil-millis())>0;
}
void monitorManualOverride(){
  // User-directed diagnostic sessions win over the background scheduler.
  monHoldUntil=millis()+300000UL;
  monOwnSession=false;
}
void monitorPersistedSample(uint8_t mod){
  if(mod<6)monModules[mod].recentWrites++;
}
void monitorLog(const String &type,ModuleId mod,const String &detail){
  if(!sdCardReady())return;
  File f=SD.open("/t1nlogs/monitor_events.csv","a");
  if(!f){monLastError="Cannot append monitor events on SD";return;}
  const uint32_t t=historyNow();
  f.print(t);f.print(',');f.print(millis());f.print(',');
  f.print((int)mod);f.print(',');f.print(type);f.print(',');
  // bounded, single-line output. Do not log private configuration data.
  String safe=detail.substring(0,95);
  safe.replace(",",";");safe.replace("\r"," ");safe.replace("\n"," ");
  f.println(safe);f.flush();f.close();
}
String monitorEncodeDtc(const MonModule &m){
  String s="";
  for(uint8_t i=0;i<m.dtcCount;i++){
    char item[7];snprintf(item,sizeof(item),"%04X%02X",m.codes[i],m.statuses[i]);s+=item;
  }
  return s;
}
void monitorLoadDtcSnapshots(){
  Preferences p;
  if(!p.begin("t1ndtc",true))return;
  for(uint8_t mod: {0,2}){
    String raw=p.getString(mod==0?"cr2":"egs","");
    if(raw.length()%6 || raw.length()>192)continue;
    MonModule &m=monModules[mod];
    m.dtcCount=raw.length()/6;
    for(uint8_t i=0;i<m.dtcCount;i++){
      m.codes[i]=(uint16_t)strtoul(raw.substring(i*6,i*6+4).c_str(),nullptr,16);
      m.statuses[i]=(uint8_t)strtoul(raw.substring(i*6+4,i*6+6).c_str(),nullptr,16);
    }
    // An empty snapshot is still a valid baseline after a successful read.
    m.dtcBaseline=p.getBool(mod==0?"cr2valid":"egsvalid",false);
  }
  p.end();
}
void monitorSaveDtcSnapshot(uint8_t mod){
  Preferences p;if(!p.begin("t1ndtc",false))return;
  p.putString(mod==0?"cr2":"egs",monitorEncodeDtc(monModules[mod]));
  p.putBool(mod==0?"cr2valid":"egsvalid",true);p.end();
}
void monitorDtcEvent(uint8_t mod,uint16_t code,uint8_t status,const char *change){
  monDtcEvents++;
  if(sdCardReady()){
    File f=SD.open("/t1nlogs/dtc_events.csv","a");
    if(f){
      const uint32_t epoch=historyNow();
      f.print(epoch);f.print(',');f.print(millis());f.print(',');
      f.print(mod);f.print(',');f.print(change);f.print(',');
      char raw[7];snprintf(raw,sizeof(raw),"%04X%02X",code,status);
      f.println(raw);f.flush();f.close();
    }else monLastError="DTC event SD write failed";
  }
}
void monitorDtcSnapshot(uint8_t mod,const uint16_t *codes,const uint8_t *statuses,uint8_t count){
  if((mod!=0&&mod!=2)||count>32)return;
  MonModule &m=monModules[mod];
  MonitorDtcItem previous[32],current[32];
  for(uint8_t j=0;j<m.dtcCount;j++)previous[j]={m.codes[j],m.statuses[j]};
  for(uint8_t i=0;i<count;i++)current[i]={codes[i],statuses[i]};
  MonitorDtcChange events[64];
  size_t n=monitorDiffDtc(previous,m.dtcCount,m.dtcBaseline,
                          current,count,events,64);
  bool changed=!m.dtcBaseline || count!=m.dtcCount || n!=0;
  for(size_t i=0;i<n;i++){
    const char *kind=events[i].kind==MON_DTC_NEW?"NEW":
                     events[i].kind==MON_DTC_STATUS_CHANGED?"STATUS_CHANGED":
                     events[i].kind==MON_DTC_NOT_REPORTED?"NOT_REPORTED":"BASELINE";
    monitorDtcEvent(mod,events[i].code,events[i].status,kind);
  }
  m.dtcCount=count;
  for(uint8_t i=0;i<count;i++){m.codes[i]=codes[i];m.statuses[i]=statuses[i];}
  m.dtcBaseline=true;
  m.lastDtcUptimeMs=millis();m.lastDtcEpoch=historyNow();
  if(changed)monitorSaveDtcSnapshot(mod);
  // Unsupported/failed reads never enter this function.
}
void monitorBegin(){
  Preferences p;
  if(p.begin("t1nmonitor",true)){monEnabled=p.getBool("enabled",true);p.end();}
  monitorLoadDtcSnapshots();
  uint32_t now=millis();
  monRetryMs=now+12000UL;monEgsDue=now+240000UL;
  monAbsDue=now+600000UL;monHzrDue=now+900000UL;
}
void monitorSetEnabled(bool enable){
  monEnabled=enable;
  if(!enable){monOwnSession=false;monTarget=MOD_ENGINE;}
  else {monRetryMs=millis()+1000UL;monTarget=MOD_ENGINE;}
  Preferences p;if(p.begin("t1nmonitor",false)){p.putBool("enabled",enable);p.end();}
}
void monitorStart(ModuleId mod){
  // Never switch during an active diagnostic/test operation.
  if(busy||jobConnect||hzrFanOverrideActive)return;
  if(diagnosticSession)disconnectCurrentModule();
  autoReconnect=false;reconnectAtMs=0;
  requestedModule=mod;
  bool ok=connectModule(mod);
  uint32_t now=millis();
  if(!ok){
    monModules[(uint8_t)mod].failures++;
    monLastError=lastError;
    monitorLog("CONNECT_FAILED",mod,lastError);
    // A failed secondary visit must not leave the engine offline for 30 s.
    monTarget=MOD_ENGINE;
    monRetryMs=now+(mod==MOD_ENGINE?30000UL:1000UL);
    monOwnSession=false;
    return;
  }
  monTarget=mod;monOwnSession=true;monStartMs=now;
  MonModule &m=monModules[(uint8_t)mod];
  m.connects++;m.lastConnectionMs=now;
  monitorLog("CONNECTED",mod,moduleName(mod));
  if(mod==MOD_ENGINE||mod==MOD_EGS){
    // Only previously verified service 18 paths are used.
    readDTCs();lastDtcScanMs=millis();
  }
}
void monitorTick(){
  // A heartbeat records whether real UTC was available when monitoring ran.
  // The clock API clearly marks hard power-loss recovery as unsupported.
  if(sdCardReady() && millis()-monLastClockLogMs>=300000UL){
    monLastClockLogMs=millis();
    monitorLog("CLOCK",monTarget,
      String("epoch=")+historyNow()+";source="+historyClockJson());
  }
  if(!monEnabled||monitorManualHold()||busy||jobConnect||hzrFanOverrideActive)return;
  uint32_t now=millis();
  if(!monOwnSession&&diagnosticSession){
    // Existing manually initiated session: don't seize it until disconnect.
    return;
  }
  if(!diagnosticSession){
    // A dropped session is not a clean scheduled disconnect. Back off
    // instead of retrying on every 2-ms loop iteration.
    if(monOwnSession){
      monModules[(uint8_t)monTarget].failures++;
      monitorLog("SESSION_LOST",monTarget,lastError);
      monOwnSession=false;
      monTarget=MOD_ENGINE;
      monRetryMs=now+10000UL;
    }
    if((int32_t)(now-monRetryMs)>=0)monitorStart(monTarget);
    return;
  }
  autoReconnect=false;reconnectAtMs=0;
  if(activeModule!=monTarget)return;
  if(activeModule!=MOD_ENGINE){
    uint32_t limit=activeModule==MOD_EGS?12000UL:8000UL;
    if(now-monStartMs>=limit)monitorStart(MOD_ENGINE);
    return;
  }
  // At least 120 s engine duty before any intermittent diagnostic window.
  if(now-monStartMs<120000UL)return;
  if((int32_t)(now-monEgsDue)>=0){
    monEgsDue=now+240000UL;monitorStart(MOD_EGS);return;
  }
  // Do not enter ABS diagnostic mode automatically while driving until
  // its side effects have been vehicle-qualified. A speed reading must be
  // fresh; missing/stale speed never implies that the vehicle is parked.
  bool stationary=engSpeed.valid && !stale(engSpeed,5000)
    && engSpeed.value>=0 && engSpeed.value<1.0
    && engRpm.valid && !stale(engRpm,5000) && engRpm.value>450;
  if(stationary && (int32_t)(now-monAbsDue)>=0){
    monAbsDue=now+600000UL;monitorStart(MOD_ABS);return;
  }
  if((int32_t)(now-monHzrDue)>=0){
    monHzrDue=now+900000UL;monitorStart(MOD_HZR);return;
  }
}
String monitorStatusJson(){
  String s="{\"enabled\":"+String(monEnabled?"true":"false")
    +",\"manualHold\":"+String(monitorManualHold()?"true":"false")
    +",\"active\":"+String(monOwnSession?"true":"false")
    +",\"target\":"+String((int)monTarget)
    +",\"clock\":"+historyClockJson()
    +",\"dtcEventsSinceBoot\":"+String(monDtcEvents)
    +",\"error\":\""+jsonEscape(monLastError)+"\",\"modules\":[";
  const uint8_t ids[4]={0,2,1,3};
  for(int i=0;i<4;i++){
    if(i)s+=",";
    uint8_t id=ids[i];const MonModule &m=monModules[id];
    s+="{\"id\":"+String(id)+",\"name\":\""+jsonEscape(moduleName((ModuleId)id))+"\""
      +",\"connects\":"+String(m.connects)+",\"failures\":"+String(m.failures)
      +",\"historyWrites\":"+String(m.recentWrites)
      +",\"lastConnectionUptimeMs\":"+String(m.lastConnectionMs)
      +",\"dtcSupported\":"+String((id==0||id==2)?"true":"false")
      +",\"dtcBaseline\":"+String(m.dtcBaseline?"true":"false")
      +",\"dtcLastEpoch\":"+String(m.lastDtcEpoch)
      +",\"dtcLastUptimeMs\":"+String(m.lastDtcUptimeMs)
      +",\"dtcCount\":"+String(m.dtcCount)+"}";
  }
  return s+"]}";
}

// Non-vehicle self-test: exercise each module's namespaced CRC record on the
// mounted card, then remove temporary files. This never invents ECU samples.
bool monitorStorageSelfTest(String &report){
  if(!sdCardReady()){report="microSD unavailable";return false;}
  if(busy||jobConnect||diagnosticSession){report="Disconnect ECU before SD self-test";return false;}
  uint8_t modules[]={0,2,1,3};
  for(uint8_t module:modules){
    String path="/t1nlogs/.module"+String(module)+".test";
    SD.remove(path);
    HistoryRecord item=historyMake(historyKey(module,"storage-selftest"),
      1760000000UL,(float)module,(float)module,(float)module,1);
    item.serial=1;item.checksum=historyChecksum(item);
    File f=SD.open(path,"w");
    if(!f){report="Cannot create "+path;return false;}
    bool wrote=f.write((const uint8_t*)&item,sizeof(item))==sizeof(item);
    f.flush();f.close();
    HistoryRecord readback={};
    f=SD.open(path,"r");
    bool ok=wrote&&f && f.read((uint8_t*)&readback,sizeof(readback))==sizeof(readback)
      &&memcmp(&item,&readback,sizeof(item))==0&&historyValid(readback);
    if(f)f.close();
    SD.remove(path);
    if(!ok){report="Readback/CRC failed for module "+String(module);return false;}
  }
  report="4/4 module CRC storage patterns wrote, read, verified and deleted (synthetic, not ECU data)";
  return true;
}
