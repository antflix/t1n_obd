#include <cassert>
#include <iostream>
#include <cstdlib>
#include "FS.h"
#include "Preferences.h"
#include "bounded_json.h"
fs::FS SD;constexpr const char* FILE_READ="r";
uint32_t ticks=1000,epoch=1793300000;
uint32_t millis(){return ticks;}uint32_t historyNow(){return epoch;}
bool card=true;bool sdCardReady(){return card;}
struct ValueState{bool valid=true;float value=0;uint32_t updatedMs=1000;};
bool stale(const ValueState&v){return !v.valid||ticks-v.updatedMs>24000;}
ValueState engRpm,engCoolant,engOilTemp,engBattery,engRail,engBoost,engRailTarget,engBoostTarget;
constexpr size_t CONTENT_LENGTH_UNKNOWN=~size_t(0);
struct Web{int code=0;String body;std::map<std::string,String> args;
 String arg(const char*k){return args[k];}void sendHeader(const char*,const char*){}void setContentLength(size_t){}
 void send(int c,const char*,const String&s){code=c;body=s;}void sendContent(const char*s,size_t n){body.append(s,n);}void sendContent(const char*){}
}web;
void sensorHttpSink(const char*s,size_t n,void*){web.sendContent(s,n);}
#include "39_alerts.ino"
int main(){
 alertRules[0].enabled=true;alertRules[0].trigger=100;alertRules[0].clear=95;alertRules[0].debounceMs=1000;assert(alertsSave(0));
 engCoolant.value=110;card=false;alertsTick();ticks+=1000;alertsTick();assert(alertStates[0].active&&alertOutbox.count==1);
 auto serial=alertOutbox.events[0].serial;
 // Power loss while SD is absent must preserve the pending transition and active snapshot.
 alertOutbox=AlertOutbox();alertStates[0]=AlertState();alertsBegin();assert(alertStates[0].active&&alertOutbox.count==1&&alertOutbox.events[0].serial==serial);
 card=true;alertRetryAt=0;alertsFlush();assert(alertOutbox.count==0);assert(!SD.files["/t1nlogs/alert_events.csv"].empty());
 web.args["id"]="coolant";alertsAcknowledge();assert(alertStates[0].acknowledged&&alertOutbox.count==1);alertsAcknowledge();assert(alertOutbox.count==1);
 ticks+=1000;engCoolant.value=90;alertsTick();assert(!alertStates[0].active);assert(alertOutbox.count==1&&alertOutbox.events[0].transition==1);
 Preferences::fail=true;auto before=alertOutbox.count;assert(!alertsEvent(0,"ACTIVE"));assert(alertOutbox.count==before&&alertWriteErrors>0);Preferences::fail=false;
 SD.shortWrite=true;alertRetryAt=0;alertsFlush();assert(alertOutbox.count==before);SD.shortWrite=false;ticks+=30001;alertsFlush();assert(alertOutbox.count==0);
 auto &bytes=SD.files["/t1nlogs/alert_events.csv"];assert(!bytes.empty()&&bytes.back()=='\n');
 std::string text(bytes.begin(),bytes.end());assert(text.find("RESOLVED")!=std::string::npos);
 web.args={{"id","coolant"},{"enabled","1"},{"trigger","105"},{"clear","100"},{"debounceMs","2000"},{"severity","2"}};alertsConfigure();assert(web.code==200&&alertRules[0].trigger==105);
 web.args["trigger"]="bad";alertsConfigure();assert(web.code==400&&alertRules[0].trigger==105);
 std::cout<<"Alert SD failure, NVS outbox reboot recovery, ack dedup, resolution, short write retry and configuration validation PASS\n";
}
