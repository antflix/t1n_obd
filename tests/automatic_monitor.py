"""Native tests of production automatic scheduling and manual API rejection."""
import pathlib,subprocess,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
monitor=(ROOT/'firmware/t1n_obd/38_autonomous_monitor.ino').read_text()
routes=(ROOT/'firmware/t1n_obd/90_setup.ino').read_text()
def function(src,signature):
    a=src.index(signature);i=src.index('{',a);depth=1;j=i+1
    while depth:
        depth+=(src[j]=='{')-(src[j]=='}');j+=1
    return src[a:j]
a=monitor.index('struct MonModule');b=monitor.index('\n};',a)+3
program=r'''
#include <cassert>
#include <functional>
#include <iostream>
#include "FS.h"
enum ModuleId:uint8_t {MOD_ENGINE=0,MOD_ABS=1,MOD_EGS=2,MOD_HZR=3,MOD_EWM=4,MOD_KI=5};
uint32_t ticks=0;uint32_t millis(){return ticks;}
bool busy=false,hzrFanOverrideActive=false,diagnosticSession=false,livePolling=false,respond=false;
ModuleId activeModule=MOD_ENGINE,requestedModule=MOD_ENGINE;
uint32_t lastDtcScanMs=0;unsigned snapshotsLoaded=0;String lastError;
std::vector<ModuleId> connections;
bool sdCardReady(){return false;}uint32_t historyNow(){return 0;}
String historyClockJson(){return "unknown";}String moduleName(ModuleId){return "module";}
void monitorLoadDtcSnapshots(){snapshotsLoaded++;}
void monitorLog(const String&,ModuleId,const String&){}
void disconnectCurrentModule(){diagnosticSession=false;livePolling=false;}
bool connectModule(ModuleId m){connections.push_back(m);activeModule=m;
 diagnosticSession=livePolling=respond;lastError=respond?"":"No ECU reply";return respond;}
bool readDTCs(){ticks+=10;return true;}
void monitorStart(ModuleId);
'''+monitor[a:b]+r'''
MonModule monModules[6];bool monOwnSession=false;
uint32_t monStartMs=0,monRetryMs=0,monEgsDue=0,monHzrDue=0,monLastClockLogMs=0;
ModuleId monTarget=MOD_ENGINE;String monLastError;
'''
for sig in ['void monitorBegin(', 'void monitorStart(', 'void monitorTick(', 'void monitorPersistedSample(']:program+=function(monitor,sig)+'\n'
a=routes.index(' const char* manualRoutes[]');b=routes.index('\n });',a)+len('\n });')
program+=r'''
constexpr int HTTP_POST=1;
struct FakeWeb {
 std::map<std::string,std::function<void()>> handlers;int status=0;String body;
 void on(const char* path,int,std::function<void()> action){handlers[path]=action;}
 void send(int code,const char*,const char* text){status=code;body=text;}
}web;
void registerManualRejections(){
'''+routes[a:b]+r'''
}
int main(){
 // Boot connects without any browser, UI command or persisted enabled setting.
 monitorBegin();assert(snapshotsLoaded==1);ticks=999;monitorTick();assert(connections.empty());
 ticks=1000;monitorTick();assert(connections.size()==1&&!diagnosticSession);
 // Hundreds of unsuccessful attempts never permanently disable connection.
 for(unsigned i=0;i<200;i++){
  unsigned before=connections.size();ticks=monRetryMs-1;monitorTick();assert(connections.size()==before);
  ticks=monRetryMs;monitorTick();assert(connections.size()==before+1);
  assert(connections.back()==MOD_ENGINE&&!diagnosticSession&&monRetryMs-ticks==30000);
 }
 respond=true;ticks=monRetryMs;monitorTick();assert(diagnosticSession&&livePolling&&monOwnSession&&monLastError.empty());
 monitorPersistedSample(0);assert(monModules[0].recentWrites==1);
 // A lost established session has one retry owner and a finite pause.
 diagnosticSession=livePolling=false;unsigned before=connections.size();monitorTick();
 assert(monRetryMs-ticks==10000&&connections.size()==before);
 ticks=monRetryMs;monitorTick();assert(diagnosticSession&&livePolling&&connections.size()==before+1);
 // Preserve engine priority and verified shorter secondary visits; never ABS.
 ticks=monStartMs+119999;before=connections.size();monitorTick();assert(connections.size()==before);
 ticks=monStartMs+120000;monEgsDue=ticks;monitorTick();assert(activeModule==MOD_EGS);
 ticks=monStartMs+12000;monitorTick();assert(activeModule==MOD_ENGINE);
 ticks=monStartMs+120000;monEgsDue=ticks+999999;monHzrDue=ticks;monitorTick();assert(activeModule==MOD_HZR);
 ticks=monStartMs+8000;monitorTick();assert(activeModule==MOD_ENGINE);
 for(auto id:connections)assert(id!=MOD_ABS&&id!=MOD_EWM&&id!=MOD_KI);
 // Busy work prevents takeover; the retry still recovers afterward.
 diagnosticSession=livePolling=false;monitorTick();ticks=monRetryMs;before=connections.size();busy=true;
 monitorTick();assert(connections.size()==before);busy=false;monitorTick();assert(diagnosticSession);
 // Retry deadlines remain valid across uptime rollover.
 respond=false;diagnosticSession=livePolling=monOwnSession=false;ticks=0xfffffff0U;monRetryMs=ticks;
 monitorTick();before=connections.size();ticks=monRetryMs-1;monitorTick();assert(connections.size()==before);
 ticks=monRetryMs;monitorTick();assert(connections.size()==before+1);
 // Old or directly crafted manual requests cannot change state even offline.
 registerManualRejections();assert(web.handlers.size()>=20);
 for(auto& entry:web.handlers){
  before=connections.size();auto priorTarget=monTarget;bool priorSession=diagnosticSession;
  entry.second();assert(web.status==409&&web.body.find("AUTOMATIC_MONITORING")!=String::npos);
  assert(connections.size()==before&&monTarget==priorTarget&&diagnosticSession==priorSession);
 }
 std::cout<<"Automatic monitor: browser-free boot, 200 failures/recovery, one retry owner, session loss, secondary visits, no ABS, busy/wrap and manual HTTP409 PASS\n";
}
'''
with tempfile.TemporaryDirectory(prefix='t1n-auto-monitor-') as temp:
    src=pathlib.Path(temp)/'monitor.cpp';binary=pathlib.Path(temp)/'monitor';src.write_text(program)
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-I'+str(ROOT/'tests/fakes'),str(src),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
for obsolete in ['monitorManualOverride','monitorManualHold','monHoldUntil','monEnabled','monAbsDue']:
    assert obsolete not in monitor,obsolete
assert 't1nmonitor' not in monitor
for p in (ROOT/'firmware/t1n_obd').glob('*.ino'):
    for obsolete in ['autoReconnect','reconnectAtMs','jobConnect']:
        assert obsolete not in p.read_text(),(p.name,obsolete)
mobile=(ROOT/'firmware/t1n_obd/50_scanner_ui.ino').read_text()
advanced=(ROOT/'firmware/t1n_obd/40_advanced_ui.ino').read_text()
for obsolete in ['connectActive','disconnectActive','pollOn','pollOff','testCmd','cr2TestPanel','readDtc','readGroupOnce','buildActuators']:
    assert obsolete not in mobile,obsolete
for obsolete in ['sendRaw','clearDtc','saveCfg','probeGroup','CONNECT CR2','/connect','/poll/']:
    assert obsolete not in advanced,obsolete
assert not (ROOT/'firmware/t1n_obd/25_engine_tests.ino').exists()
