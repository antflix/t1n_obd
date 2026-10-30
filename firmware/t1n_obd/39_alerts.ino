// Numeric vehicle thresholds are disabled until explicitly configured. They are
// user rules, not claims about factory fault limits. Evaluated without a browser.
#include "alert_rule.h"
#include "csv_journal.h"
static AlertRule alertRules[6];
static AlertState alertStates[6];
static const char *alertIds[]={"coolant","oilTemp","batteryLow","batteryHigh","railDeficit","boostError"};
static const char *alertUnits[]={"°C","°C","V","V","bar","hPa"};
static uint32_t alertLastTick=0,alertWriteErrors=0;
static bool alertSnapshotDirty[6]={};
bool alertsSave(unsigned i);
struct PendingAlertEvent {uint32_t epoch,uptime,serial;float evidence;uint8_t rule,transition,severity,reserved;};
struct AlertOutbox {uint32_t version=1,nextSerial=1,count=0;PendingAlertEvent events[16]={};};
static AlertOutbox alertOutbox;
static uint32_t alertRetryAt=0;
static const char *alertTransitions[]={"ACTIVE","RESOLVED","ACKNOWLEDGED","CONFIG_CHANGED"};
bool alertsSaveOutbox(){
  Preferences p;if(!p.begin("t1nalerts",false)){alertWriteErrors++;return false;}
  bool ok=p.putBytes("outbox",&alertOutbox,sizeof(alertOutbox))==sizeof(alertOutbox);p.end();if(!ok)alertWriteErrors++;return ok;
}
void alertsFlush(){
  if(!alertOutbox.count||(int32_t)(millis()-alertRetryAt)<0)return;
  for(unsigned i=0;i<6;i++)if(alertSnapshotDirty[i]&&!alertsSave(i)){alertRetryAt=millis()+30000;return;}
  if(!sdCardReady()){alertWriteErrors++;alertRetryAt=millis()+30000;return;}
  const PendingAlertEvent&e=alertOutbox.events[0];
  char row[180];int len=snprintf(row,sizeof(row),"%lu,%lu,0,%s,%s,%u,%.6g,%lu\n",(unsigned long)e.epoch,(unsigned long)e.uptime,alertIds[e.rule],alertTransitions[e.transition],e.severity,e.evidence,(unsigned long)e.serial);
  bool ok=len>0&&(size_t)len<sizeof(row)&&sdAppendCsv("/t1nlogs/alert_events.csv",row,(size_t)len);
  if(!ok){alertWriteErrors++;alertRetryAt=millis()+30000;return;}
  AlertOutbox prior=alertOutbox;
  for(unsigned j=1;j<alertOutbox.count;j++)alertOutbox.events[j-1]=alertOutbox.events[j];
  alertOutbox.count--;
  if(!alertsSaveOutbox()){alertOutbox=prior;alertRetryAt=millis()+30000;}
}
String alertEncode(unsigned i){
  const AlertState &a=alertStates[i];const AlertRule &r=alertRules[i];
  return String(r.enabled?1:0)+","+String(r.high?1:0)+","+String(r.trigger,4)+","+String(r.clear,4)+","+String(r.debounceMs)+","+String(r.severity)+","+String(a.active?1:0)+","+String(a.acknowledged?1:0)+","+String(a.firstEpoch)+","+String(a.lastEpoch);
}
bool alertsSave(unsigned i){
  Preferences p;if(!p.begin("t1nalerts",false)){alertWriteErrors++;alertSnapshotDirty[i]=true;return false;}
  bool ok=p.putString(alertIds[i],alertEncode(i))>0;p.end();alertSnapshotDirty[i]=!ok;if(!ok)alertWriteErrors++;return ok;
}
void alertsBegin(){
  Preferences p;if(!p.begin("t1nalerts",true))return;
  for(unsigned i=0;i<6;i++){
    String raw=p.getString(alertIds[i],"");unsigned en,hi,deb,sev,active,ack,first,last;float trigger,clear;
    if(sscanf(raw.c_str(),"%u,%u,%f,%f,%u,%u,%u,%u,%u,%u",&en,&hi,&trigger,&clear,&deb,&sev,&active,&ack,&first,&last)!=10)continue;
    AlertRule r;r.enabled=en==1;r.high=hi==1;r.trigger=trigger;r.clear=clear;r.debounceMs=deb;r.severity=sev;
    if(!r.valid())continue;
    alertRules[i]=r;alertStates[i].active=active==1;alertStates[i].acknowledged=ack==1;alertStates[i].firstEpoch=first;alertStates[i].lastEpoch=last;
  }
  if(p.getBytesLength("outbox")==sizeof(alertOutbox)){
    AlertOutbox saved;if(p.getBytes("outbox",&saved,sizeof(saved))==sizeof(saved)&&saved.version==1&&saved.count<=16&&saved.nextSerial){
      bool valid=true;for(unsigned i=0;i<saved.count;i++)if(saved.events[i].rule>=6||saved.events[i].transition>=4||!isfinite(saved.events[i].evidence))valid=false;
      if(valid)alertOutbox=saved;else alertWriteErrors++;
    }
  }
  p.end();
  // Reconcile durable transitions with a snapshot interrupted during NVS save.
  for(unsigned i=0;i<alertOutbox.count;i++){
    const PendingAlertEvent&e=alertOutbox.events[i];AlertState&a=alertStates[e.rule];
    if(e.transition==0){a.active=true;a.acknowledged=false;a.firstEpoch=e.epoch;}
    else if(e.transition==1)a.active=false;
    else if(e.transition==2)a.acknowledged=true;
    a.evidence=e.evidence;a.lastEpoch=e.epoch;alertSnapshotDirty[e.rule]=true;
  }
}
bool alertsEvent(unsigned i,const char *transition){
  if(alertOutbox.count>=16||alertOutbox.nextSerial==0){alertWriteErrors++;return false;}
  unsigned kind=0;for(unsigned j=0;j<4;j++)if(strcmp(transition,alertTransitions[j])==0)kind=j;
  AlertOutbox prior=alertOutbox;
  alertOutbox.events[alertOutbox.count++]={historyNow(),millis(),alertOutbox.nextSerial++,alertStates[i].evidence,(uint8_t)i,(uint8_t)kind,alertRules[i].severity,0};
  if(!alertsSaveOutbox()){alertOutbox=prior;return false;}
  return true;
}
void alertsTick(){
  uint32_t now=millis();if(now-alertLastTick<1000)return;alertLastTick=now;alertsFlush();
  const bool running=engRpm.valid&&!stale(engRpm)&&engRpm.value>450;
  const ValueState* values[]={&engCoolant,&engOilTemp,&engBattery,&engBattery,&engRail,&engBoost};
  for(unsigned i=0;i<6;i++){
    const ValueState &v=*values[i];bool valid=v.valid&&!stale(v);
    float evidence=v.value;
    if(i==2||i==3)valid=valid&&running;
    if(i>=4){const ValueState &target=i==4?engRailTarget:engBoostTarget;
      uint32_t va=now-v.updatedMs,ta=now-target.updatedMs;
      valid=valid&&running&&target.valid&&!stale(target)&&(va>ta?va-ta:ta-va)<=3000;
      evidence=i==4?target.value-v.value:fabsf(target.value-v.value);
    }
    AlertState &a=alertStates[i];AlertState prior=a;AlertTransition tr=a.update(alertRules[i],now,valid,evidence);
    if(tr!=ALERT_UNCHANGED){
      a.lastEpoch=historyNow();if(tr==ALERT_OPENED)a.firstEpoch=a.lastEpoch;
      // Persist the event outbox before changing the saved active snapshot.
      if(alertsEvent(i,a.active?"ACTIVE":"RESOLVED"))alertsSave(i);
      else {a=prior;a.available=false;}
    }
  }
}
void alertsSend(){
  web.sendHeader("Cache-Control","no-store");web.setContentLength(CONTENT_LENGTH_UNKNOWN);web.send(200,"application/json","");BoundedJson out(sensorHttpSink);
  out.raw("{\"ok\":true,\"writeErrors\":");out.integer(alertWriteErrors);out.raw(",\"pendingEvents\":");out.integer(alertOutbox.count);out.raw(",\"rules\":[");
  for(unsigned i=0;i<6;i++){
    const AlertState&a=alertStates[i];const AlertRule&r=alertRules[i];
    if(i)out.raw(",");
    out.raw("{\"id\":");out.string(alertIds[i]);out.raw(",\"module\":0,\"unit\":");out.string(alertUnits[i]);
    out.raw(",\"enabled\":");out.raw(r.enabled?"true":"false");out.raw(",\"high\":");out.raw(r.high?"true":"false");
    out.raw(",\"trigger\":");out.number(r.trigger);out.raw(",\"clear\":");out.number(r.clear);out.raw(",\"debounceMs\":");out.integer(r.debounceMs);out.raw(",\"severity\":");out.integer(r.severity);
    out.raw(",\"active\":");out.raw(a.active?"true":"false");out.raw(",\"acknowledged\":");out.raw(a.acknowledged?"true":"false");out.raw(",\"available\":");out.raw(a.available?"true":"false");
    out.raw(",\"evidence\":");out.number(a.evidence);out.raw(",\"firstEpoch\":");out.integer(a.firstEpoch);out.raw(",\"lastEpoch\":");out.integer(a.lastEpoch);out.raw("}");
  }
  out.raw("]}");out.flush();web.sendContent("");
}
void alertsConfigure(){
  int index=-1;for(unsigned i=0;i<6;i++)if(web.arg("id")==alertIds[i])index=i;
  if(index<0){web.send(400,"text/plain","Unknown alert rule");return;}
  if(web.arg("enabled")!="0"&&web.arg("enabled")!="1"){web.send(400,"text/plain","Set enabled=0 or 1");return;}
  AlertRule r=alertRules[index];r.enabled=web.arg("enabled")=="1";
  if(r.enabled){
    char *end=nullptr;String triggerText=web.arg("trigger"),clearText=web.arg("clear");r.trigger=strtof(triggerText.c_str(),&end);
    if(!web.arg("trigger").length()||!end||*end){web.send(400,"text/plain","Invalid trigger");return;}
    r.clear=strtof(clearText.c_str(),&end);
    if(!web.arg("clear").length()||!end||*end){web.send(400,"text/plain","Invalid clear threshold");return;}
    String deb=web.arg("debounceMs"),sev=web.arg("severity");
    if(!deb.length()||!sev.length()){web.send(400,"text/plain","Missing persistence or severity");return;}
    r.debounceMs=strtoul(deb.c_str(),&end,10);if(!end||*end){web.send(400,"text/plain","Invalid debounce");return;}
    unsigned severity=strtoul(sev.c_str(),&end,10);if(!end||*end||severity>2){web.send(400,"text/plain","Invalid severity");return;}r.severity=severity;
    r.high=index!=2;
    if(!r.valid()){web.send(400,"text/plain","Require hysteresis and debounce <=600000 ms");return;}
  }
  if(alertStates[index].active&&!alertsEvent(index,"CONFIG_CHANGED")){web.send(503,"text/plain","Could not persist rule change event");return;}
  AlertRule oldRule=alertRules[index];AlertState oldState=alertStates[index];
  alertRules[index]=r;alertStates[index]=AlertState();
  if(!alertsSave(index)){alertRules[index]=oldRule;alertStates[index]=oldState;web.send(503,"text/plain","Could not persist rule settings");return;}alertsSend();
}
void alertsAcknowledge(){
  for(unsigned i=0;i<6;i++)if(web.arg("id")==alertIds[i]){
    if(!alertStates[i].active){web.send(409,"text/plain","Alert is not active");return;}
    if(!alertStates[i].acknowledged){if(!alertsEvent(i,"ACKNOWLEDGED")){web.send(503,"text/plain","Could not persist acknowledgment");return;}alertStates[i].acknowledged=true;alertsSave(i);}alertsSend();return;
  }
  web.send(400,"text/plain","Unknown alert");
}
