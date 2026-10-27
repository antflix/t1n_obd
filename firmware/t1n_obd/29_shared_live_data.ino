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
 {MOD_EWM,0x2A,0x01,0x01,3,"EWM selector and switch measurements",true},
 {MOD_EWM,0x2A,0x01,0x02,3,"EWM potentiometers and learned positions",true},
 {MOD_EWM,0xA1,0,0,1,"EWM fault/status record (raw, on-demand)",false},
 {MOD_EWM,0x3C,0x00,0,2,"EWM identification/production data (read once)",false}
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

// These positions are copied from the ORIGINAL EWM210.cbf DT_* service objects,
// not inferred from the values observed on this stationary vehicle.
// Bit positions are measured from the first positive-response service byte
// (6A=bit 0); kind: 0=raw8, 1=flag, 2=raw16 BE, 3=19mV/count, 4=69mV/count.
struct EwmFieldDef{uint8_t group,bit,kind;const char *id,*label;};
static const EwmFieldDef ewmFieldDefs[]={
 {1,16,0,"ewmSelector","Selector position (raw enum)"},
 {1,24,0,"ewmSupplyRaw","Supply voltage (raw)"},
 {1,32,1,"ewmKickdown","Kickdown switch"},
 {1,33,0,"ewmProgram","Winter/Summer program (raw enum)"},
 {1,34,1,"ewmPlus","Selector + switch"},
 {1,35,1,"ewmMinus","Selector - switch"},
 {1,36,1,"ewmReverseKL15","Reverse-light terminal 15"},
 {1,37,1,"ewmReverse","Reverse-light output"},
 {1,40,1,"ewmLockActual","Shift-lock solenoid actual"},
 {1,41,1,"ewmLockTarget","Shift-lock solenoid commanded"},
 {1,46,1,"ewmShiftlock","Shift-lock state"},
 {1,48,0,"ewmCan230","CAN 0x230 raw status"},
 {1,56,2,"ewmFilteredRear","Rear speed filtered (raw16)"},
 {1,72,2,"ewmWheelRR","Right rear wheel CAN speed (raw16)"},
 {1,88,2,"ewmWheelRL","Left rear wheel CAN speed (raw16)"},
 {1,104,0,"ewmHw","Hardware version raw"},
 {1,112,0,"ewmVariant","Variant raw"},
 {1,120,0,"ewmSw","Software version raw"},
 {1,128,0,"ewmData","Data version raw"},
 {1,136,2,"ewmBrakeTorque","Brake torque CAN raw16"},
 {1,144,1,"ewmDriverBrake","Driver brake signal"},
 {1,150,1,"ewmBrake","Brake input"},
 {2,16,0,"ewmPotAngle","Potentiometer angle raw"},
 {2,16,3,"ewmPotV","Potentiometer voltage"},
 {2,24,4,"ewmLockV","Shift-lock analog voltage"},
 {2,32,4,"ewmKickV","Kickdown analog voltage"},
 {2,40,0,"ewmRestD","D default/rest angle raw"},
 {2,48,0,"ewmMaxLearn","Maximum learned range raw"},
 {2,56,0,"ewmUpperP","P upper learned limit"},
 {2,64,0,"ewmLowerP","P lower learned limit"},
 {2,72,0,"ewmUpperR","R upper learned limit"},
 {2,80,0,"ewmLowerR","R lower learned limit"},
 {2,88,0,"ewmUpperN","N upper learned limit"},
 {2,96,0,"ewmLowerN","N lower learned limit"},
 {2,104,0,"ewmUpperD","D upper learned limit"},
 {2,112,0,"ewmLowerD","D lower learned limit"},
 {2,120,3,"ewmPotFaultV","Potentiometer fault-check voltage"},
 {2,128,3,"ewmPotSupplyV","Potentiometer supply voltage"},
 {2,136,0,"ewmReverseFeedback","Reverse light output feedback raw"},
 {2,144,0,"ewmLockFeedback","Shift-lock feedback raw"},
 {2,152,0,"ewmPotErrors","Potentiometer wiper error count"}
};
static const int EWM_FIELD_COUNT=sizeof(ewmFieldDefs)/sizeof(ewmFieldDefs[0]);
ValueState ewmFields[EWM_FIELD_COUNT];
void decodeEwmGroup(uint8_t group,const uint8_t *p,size_t n){
  if(n<20||p[0]!=0x6A||p[1]!=group)return;
  for(int i=0;i<EWM_FIELD_COUNT;i++){
    const EwmFieldDef &f=ewmFieldDefs[i];if(f.group!=group)continue;
    unsigned byte=f.bit/8,shift=f.bit%8;
    if(byte>=n)continue;
    if(f.kind==1)setTxt(ewmFields[i],(p[byte]&(1U<<shift))?"On":"Off");
    else if(f.kind==2){if(byte+1<n)setNum(ewmFields[i],((uint16_t)p[byte]<<8)|p[byte+1],"raw16",0);}
    else if(f.kind==3)setNum(ewmFields[i],p[byte]*0.019,"V",3);
    else if(f.kind==4)setNum(ewmFields[i],p[byte]*0.069,"V",3);
    else setNum(ewmFields[i],p[byte],"raw",0);
  }
}
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
  if(p[0]!=expected || (g.len>1 && p[1]!=(g.len==3?g.b:g.a))){s.problem="Unexpected response service/group";s.raw=bytesHex(r,rn);return false;}
  s.raw=bytesHex(r,rn);s.problem="";s.sampleMs=millis();
  // For 21 GG replies drop 61 GG; for 2A 01 GG replies drop 6A 01,
  // preserve 07 (group identifier when returned) as a visible raw byte.
  int header=(g.module==MOD_EWM && g.service==0xA1)?1:2;
  s.count=(uint8_t)min((int)n-header,48);
  for(int j=0;j<s.count;j++)setNum(s.bytes[j],p[j+header],"raw",0);
  if(g.module==MOD_EWM && g.service==0x2A)decodeEwmGroup(g.b,p,n);
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
  nextPollMs=millis()+(activeModule==MOD_EWM?3000:450);
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
      String(m==MOD_ABS?"transport research":m==MOD_EWM?"CBF-decoded selectors, inputs, analog data":m==MOD_HZR||m==MOD_KI?"verified groups + raw values":"decoded fields")+"\",\"pollable\":"+
      String(m!=MOD_ABS?"true":"false")+"}";
  }
  return out+"]}";
}
