// Read-only live-data groups confirmed in this van's Autel capture and CBF notes.
// Do not infer physical meaning from an unlabelled response byte.
// ECU-specific group request tables and raw byte telemetry are intentionally separate
// from the already validated CR2/EGS and experimental ABS transport.
struct SharedLiveGroup { uint8_t module,service,a,b,len;const char *label;bool poll; };
static const SharedLiveGroup sharedLiveGroups[]={
 {MOD_HZR,0x21,0x60,0,2,"HVAC temperature sensors",true},
 {MOD_HZR,0x21,0x22,0,2,"HVAC switch states",true},
 {MOD_HZR,0x21,0x62,0,2,"HVAC supply and compressor",true},
 {MOD_HZR,0x21,0x64,0,2,"HVAC commanded outputs",true},
 {MOD_KI,0x2A,0x01,0x01,3,"Cluster fuel, temperature and speed",true},
 {MOD_KI,0x2A,0x01,0x07,3,"Cluster input and lamp states",true},
 {MOD_HZR,0x21,0x61,0,2,"HVAC optional setpoint/voltage",false},
 {MOD_HZR,0x21,0x63,0,2,"HVAC optional refrigerant pressure",false},
 {MOD_HZR,0x21,0xE0,0,2,"HVAC optional internal values",false},
 {MOD_EWM,0xA1,0,0,1,"Shift lever fault/status summary",false}
};
static const int SHARED_GROUP_COUNT=sizeof(sharedLiveGroups)/sizeof(sharedLiveGroups[0]);
struct SharedGroupData {
  ValueState bytes[48];
  uint8_t count=0;
  bool unsupported=false;
  uint32_t sampleMs=0;
  String raw,problem;
};
SharedGroupData sharedGroupData[SHARED_GROUP_COUNT];
ValueState kiFuelLitres,kiOutsideTemp,kiDimmer,kiSpeed,kiSenderOhms;
ValueState hzTemp[6],hzSwitch[6],hzStatus[12];
int sharedPollIndex=0;
String sharedGroupRequestHex(int i){
  const SharedLiveGroup &g=sharedLiveGroups[i];
  uint8_t q[3]={g.service,g.a,g.b};
  return bytesHex(q,g.len);
}
bool readSharedGroup(int idx,bool annotateFailure=true){
  if(idx<0||idx>=SHARED_GROUP_COUNT || activeModule!=(ModuleId)sharedLiveGroups[idx].module || !diagnosticSession)return false;
  SharedGroupData &s=sharedGroupData[idx];const SharedLiveGroup &g=sharedLiveGroups[idx];
  uint8_t q[3]={g.service,g.a,g.b},r[256];size_t rn=0;
  bool ok=sendKwpPayload(q,g.len,r,rn,1000);
  if(!ok){s.problem=lastError;s.raw=lastRxHex;return false;}
  const uint8_t *p=nullptr;size_t n=0;
  if(!responsePayload(r,rn,p,n)||n<2){s.problem="Invalid or short response";s.raw=bytesHex(r,rn);return false;}
  uint8_t expected=(uint8_t)(g.service+0x40);
  if(p[0]==0x7F && n>=3){s.problem="ECU rejected request, NRC 0x"+hex2(p[2]);s.raw=bytesHex(r,rn);
    if(p[2]==0x11||p[2]==0x12||p[2]==0x31)s.unsupported=true;
    return false;
  }
  if(p[0]!=expected || (g.module!=MOD_EWM && p[1]!=g.a)){s.problem="Unexpected response service/group";s.raw=bytesHex(r,rn);return false;}
  s.raw=bytesHex(r,rn);s.problem="";s.sampleMs=millis();
  // For 21 GG replies drop 61 GG; for 2A 01 GG replies drop 6A 01,
  // preserve 07 (group identifier when returned) as a visible raw byte.
  int header=g.module==MOD_EWM?1:2;
  s.count=(uint8_t)min((int)n-header,48);
  for(int j=0;j<s.count;j++)setNum(s.bytes[j],p[j+header],"raw",0);
  if(g.module==MOD_HZR && n>=3){
    auto flagState=[&](ValueState &dst,uint8_t mask){setTxt(dst,(p[2]&mask)?"On":"Off");};
    if(g.a==0x60 && n>=8){
      setNum(hzTemp[0],p[2]*0.5-40,"°C",1);
      setNum(hzTemp[1],p[3]/6.0,"°C",1);
      setNum(hzTemp[2],p[4]*0.5-10,"°C",1);
      setNum(hzTemp[3],p[5]*(-0.4260089695)+97.9820633,"°C",1);
      setNum(hzTemp[4],p[6]-40.0,"°C",0);
      setNum(hzTemp[5],p[7],"raw",0);
    }else if(g.a==0x22){
      for(int i=0;i<6;i++)flagState(hzSwitch[i],1U<<i);
    }else if(g.a==0x62&&n>=6){
      setNum(hzStatus[0],p[2],"%",0);
      setNum(hzStatus[1],((uint16_t)p[3]<<8)|p[4],"rpm",0);
      setTxt(hzStatus[2],(p[5]&0x01)?"On":"Off");
      setTxt(hzStatus[3],(p[5]&0x80)?"On":"Off");
    }else if(g.a==0x64&&n>=5){
      for(int i=0;i<7;i++)flagState(hzStatus[4+i],1U<<i);
      setNum(hzStatus[11],(((uint16_t)p[3]<<8)|p[4])*0.0977516994,"%",1);
    }
  }
  if(g.module==MOD_KI && g.b==0x01 && n>=10){
    // INSBR901 CBF; positions validated against physical Autel snapshots.
    setNum(kiFuelLitres,p[2]*0.5,"L",1);
    int8_t t=(int8_t)p[3];
    if(t!=-127&&t!=-128)setNum(kiOutsideTemp,t*0.5,"°C",1);
    setNum(kiDimmer,p[4]*0.5,"%",1);
    setNum(kiSpeed,p[7],"km/h",0);
    setNum(kiSenderOhms,(uint16_t)p[8]|((uint16_t)p[9]<<8),"ohm",0);
  }
  if(annotateFailure)activity(String(g.label)+" received "+s.raw);
  return true;
}
void sharedLivePollTick(){
  if(!livePolling || !diagnosticSession || busy || (int32_t)(millis()-nextPollMs)<0)return;
  int available[SHARED_GROUP_COUNT],count=0;
  for(int i=0;i<SHARED_GROUP_COUNT;i++)
    if(sharedLiveGroups[i].module==(uint8_t)activeModule &&
       sharedLiveGroups[i].poll && !sharedGroupData[i].unsupported)available[count++]=i;
  if(!count){livePolling=false;return;}
  int idx=available[(sharedPollIndex++)%count];
  busy=true;bool ok=readSharedGroup(idx,false);busy=false;
  if(!ok)logx("Shared live "+sharedGroupRequestHex(idx)+" did not provide data: "+sharedGroupData[idx].problem);
  nextPollMs=millis()+450;
}
String sharedGroupCatalogJson(){
  String out="{\"module\":\""+jsonEscape(moduleName(activeModule))+"\",\"groups\":[";
  bool first=true;
  for(int i=0;i<SHARED_GROUP_COUNT;i++){
    if(sharedLiveGroups[i].module!=(uint8_t)activeModule)continue;
    if(!first)out+=",";first=false;
    out+="{\"id\":\""+sharedGroupRequestHex(i)+"\",\"label\":\""+
      jsonEscape(sharedLiveGroups[i].label)+"\",\"raw\":\""+
      jsonEscape(sharedGroupData[i].raw)+"\",\"lastMs\":"+String(sharedGroupData[i].sampleMs)+
      ",\"unsupported\":"+String(sharedGroupData[i].unsupported?1:0)+
      ",\"problem\":\""+jsonEscape(sharedGroupData[i].problem)+"\"}";
  }
  return out+"]}";
}
String readSharedGroupByRequest(const String &q,bool&ok){
  ok=false;
  String key=q;key.trim();key.toUpperCase();
  for(int i=0;i<SHARED_GROUP_COUNT;i++){
    if(activeModule!=(ModuleId)sharedLiveGroups[i].module||sharedGroupRequestHex(i)!=key)continue;
    ok=readSharedGroup(i);
    return String(sharedLiveGroups[i].label)+"\nTX: "+key+"\nRX: "+
      sharedGroupData[i].raw+(ok?"\nPositive response":"\n"+sharedGroupData[i].problem);
  }
  return "Group not in this module's verified read-only request catalog";
}
String modulesCatalogJson(){
  String out="{\"modules\":[";
  for(int i=0;i<=MOD_KI;i++){
    ModuleId m=(ModuleId)i;if(i)out+=",";
    int ch=relayIndexForModule(m);
    out+="{\"id\":"+String(i)+",\"name\":\""+jsonEscape(moduleName(m))+
      "\",\"address\":\"0x"+hex2(moduleAddr(m))+"\",\"obdPin\":"+String(moduleObdPin(m))+
      ",\"channel\":"+String(ch+1)+",\"verified\":true,\"liveCapability\":\""+
      String(m==MOD_ABS?"transport research":m==MOD_EWM?"status only":m==MOD_HZR||m==MOD_KI?"verified groups + raw values":"decoded fields")+"\",\"pollable\":"+
      String(m!=MOD_ABS&&m!=MOD_EWM?"true":"false")+"}";
  }
  return out+"]}";
}
