// Exercise the production scheduler, not a reimplementation of its decisions.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>
#include "FS.h"
#include "Preferences.h"

enum ModuleId:uint8_t {MOD_ENGINE=0,MOD_ABS=1,MOD_EGS=2,MOD_HZR=3,MOD_EWM=4,MOD_KI=5};
fs::FS SD;
uint32_t ticks=1000,lastDtcScanMs=0,reconnectAtMs=0;
uint32_t millis(){return ticks;}
bool busy=false,jobConnect=false,hzrFanOverrideActive=false;
bool diagnosticSession=false,autoReconnect=false,hReady=true;
ModuleId requestedModule=MOD_ENGINE,activeModule=MOD_ENGINE;
String lastError;
bool sdCardReady(){return true;}
uint32_t historyNow(){return 1793400000;}
String historyClockJson(){return "{\"valid\":true}";}
String jsonEscape(const String&s){return s;}
String moduleName(ModuleId m){return String(static_cast<int>(m));}

unsigned disconnects=0,dtcScans=0;
bool nextConnectionSucceeds=true;
std::vector<ModuleId> attempts;
std::function<void()> duringDisconnect,duringConnect;
void disconnectCurrentModule(){
  disconnects++;
  if(duringDisconnect)duringDisconnect();
  diagnosticSession=false;
}
bool connectModule(ModuleId id){
  attempts.push_back(id);
  // The real fast initializer clears busy before CR2 verification completes.
  busy=false;
  if(duringConnect)duringConnect();
  diagnosticSession=nextConnectionSucceeds;
  if(diagnosticSession)activeModule=id;
  else lastError="Fixture: no ECU response";
  return diagnosticSession;
}
bool readDTCs(){dtcScans++;return true;}

// Link-only storage-selftest dependencies. The tested scheduler runs the real
// monitorLog against the faultable in-memory SD; storage selftests are separate.
struct HistoryRecord {uint32_t serial=0,key=0;};
struct HistoryRing {const char *path;uint16_t capacity,used=0,next=0;};
String historyPath(const char *path){return String(path);}
uint32_t historyKey(uint8_t mod,const char*){return mod+1;}
HistoryRecord historyMake(uint32_t key,uint32_t,float,float,float,uint32_t){HistoryRecord r;r.key=key;return r;}
bool historyWrite(HistoryRing&,uint16_t,HistoryRecord&,bool){return false;}
bool historyRead(HistoryRing&,uint16_t,HistoryRecord&){return false;}
constexpr const char *FILE_READ="r";

#include "38_autonomous_monitor.ino"

void reset(){
  ticks=1000;busy=jobConnect=hzrFanOverrideActive=false;
  diagnosticSession=autoReconnect=false;reconnectAtMs=0;
  nextConnectionSucceeds=true;disconnects=dtcScans=0;
  attempts.clear();duringDisconnect=duringConnect={};
  Preferences::booleans.clear();Preferences::strings.clear();SD.files.clear();
  monMode=MonitorOwnership();monRetryMs=monStartMs=monLastClockLogMs=0;
  monEgsDue=monHzrDue=0;monTarget=MOD_ENGINE;monLastError="";
  for(auto &module:monModules)module=MonModule();
}

int main(){
  reset();monitorBegin();ticks+=20000;monitorTick();
  assert(!monitorOwnsUart()&&attempts.empty()); // default stays paused

  reset();Preferences::booleans["t1nmonitor/enabled"]=true;monitorBegin();
  assert(monitorOwnsUart()&&!diagnosticSession);
  ticks=12999;monitorTick();assert(attempts.empty());
  duringConnect=[](){
    assert(jobConnect&&!busy&&monitorOwnsUart());
    monitorSetEnabled(false); // nested HTTP pause must be rejected mid-connect
    assert(monitorOwnsUart());
  };
  ticks=13000;monitorTick();
  assert(attempts.size()==1&&attempts.back()==MOD_ENGINE);
  assert(monMode.ownsSession&&diagnosticSession&&!jobConnect&&dtcScans==1);
  duringConnect={};
  monitorSetEnabled(true);
  assert(disconnects==0&&monMode.ownsSession); // repeated enable preserves session

  duringDisconnect=[](){
    assert(busy);
    monitorSetEnabled(true); // nested resume cannot enter during explicit pause
    assert(!monitorOwnsUart());
  };
  monitorSetEnabled(false);
  assert(!monitorOwnsUart()&&!diagnosticSession&&!monMode.ownsSession);
  assert(disconnects==1&&!Preferences::booleans["t1nmonitor/enabled"]);
  duringDisconnect={};
  ticks+=600000;monitorTick();assert(attempts.size()==1); // no five-minute hold
  monMode=MonitorOwnership();monitorBegin();assert(!monitorOwnsUart());

  reset();diagnosticSession=true;autoReconnect=true;reconnectAtMs=9999;
  duringDisconnect=[](){assert(busy&&monitorOwnsUart());};
  monitorSetEnabled(true);
  assert(disconnects==1&&!diagnosticSession&&monitorOwnsUart());
  assert(!autoReconnect&&reconnectAtMs==0&&monRetryMs==2000);
  duringDisconnect={};ticks=2000;monitorTick();
  assert(diagnosticSession&&monMode.ownsSession);
  // The qualification guard applies at the actual connection entry point.
  auto before=attempts.size();monitorStart(MOD_ABS);monitorStart(MOD_KI);monitorStart(MOD_EWM);
  assert(attempts.size()==before&&disconnects==1);

  // Engine/EGS scheduling and exact existing retry intervals remain intact.
  ticks=monStartMs+120000;monEgsDue=ticks;monHzrDue=ticks+900000;monitorTick();
  assert(activeModule==MOD_EGS&&attempts.back()==MOD_EGS);
  ticks=monStartMs+12000;monitorTick();assert(activeModule==MOD_ENGINE);
  nextConnectionSucceeds=false;diagnosticSession=false;ticks+=1000;
  monitorTick();assert(monRetryMs==ticks+10000&&monitorOwnsUart());
  ticks=monRetryMs-1;before=attempts.size();monitorTick();assert(attempts.size()==before);
  ticks++;monitorTick();assert(attempts.size()==before+1&&monRetryMs==ticks+30000);
  assert(monitorOwnsUart()&&!diagnosticSession&&!jobConnect);
  nextConnectionSucceeds=true;ticks=monRetryMs;monitorTick();
  assert(diagnosticSession&&monMode.ownsSession);
  for(ModuleId id:attempts)assert(id!=MOD_ABS);

  // Enabled operation must reclaim an unexpected old manual session, never
  // yield indefinitely waiting for a browser to disconnect it.
  reset();monMode.enabled=true;diagnosticSession=true;activeModule=MOD_HZR;
  monitorTick();assert(attempts.size()==1&&activeModule==MOD_ENGINE&&disconnects==1);
  // Safety controls still block an operating-mode change during an override.
  hzrFanOverrideActive=true;monitorSetEnabled(false);assert(monitorOwnsUart());

  std::puts("Production monitor boot, pause/resume, nested HTTP exclusion, scheduling and retries PASS");
}
