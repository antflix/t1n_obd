// ---------------- Live data ----------------
uint16_t be16(const uint8_t*p){return((uint16_t)p[0]<<8)|p[1];}
String metricLine(const String&name,double value,const String&unit,int decimals=1){return name+": "+String(value,decimals)+(unit.length()?" "+unit:"")+"\n";}
const uint8_t* findPositivePayload(const uint8_t*frame,size_t fn,uint8_t group,size_t&rem){const uint8_t*p=nullptr;size_t pn=0;if(!responsePayload(frame,fn,p,pn)||pn<2)return nullptr;if(p[0]!=0x61||p[1]!=group)return nullptr;rem=pn;return p;}
int16_t sbe16(const uint8_t*p){return(int16_t)be16(p);}
String decodeLiveGroup(uint8_t id,const uint8_t*frame,size_t fn){
  size_t rem=0;const uint8_t*p=findPositivePayload(frame,fn,id,rem);
  if(!p){const uint8_t*q=nullptr;size_t qn=0;if(responsePayload(frame,fn,q,qn)&&qn>=3&&q[0]==0x7F)return"Negative response: 7F "+hex2(q[1])+" "+hex2(q[2])+"\n";return"No 61 "+hex2(id)+" positive response.\n";}
  String out;auto have=[&](size_t off,size_t len){return rem>=off+len;};

  if(id==0x10){
    if(have(3,1)){double v=p[3]*0.392;setNum(engLoad,v,"%",1);out+=metricLine("Calculated load",v,"%",1);}
    if(have(5,1)){double v=p[5]-40.0;setNum(engCoolant,v,"°C",0);out+=metricLine("Coolant temp",v,"C",0);}
    if(have(7,1)){double v=p[7]*10.0;setNum(engBoost,v,"hPa",0);out+=metricLine("Boost pressure",v,"hPa",0);}
    if(have(8,2)){double v=be16(p+8)*0.25;setNum(engRpm,v,"rpm",0);out+=metricLine("RPM",v,"rpm",0);}
    if(have(11,1)){double v=p[11];setNum(engSpeed,v,"km/h",0);out+=metricLine("Vehicle speed",v,"km/h",0);}
    if(have(13,1)){double v=p[13]-40.0;setNum(engIntake,v,"°C",0);out+=metricLine("Intake temp",v,"C",0);}
    if(have(14,2)){double v=be16(p+14)*0.01;setNum(engMaf,v,"g/s",2);out+=metricLine("Air mass",v,"g/s",2);}
    if(have(17,1)){double v=p[17]*0.392;setNum(engPedal1,v,"%",1);out+=metricLine("Accelerator",v,"%",1);}
  }

  if(id==0x12){
    if(have(2,2)){double v=be16(p+2)*0.1-273.14;setNum(engCoolant,v,"°C",1);out+=metricLine("Coolant temp",v,"C",1);}
    if(have(4,2)){double v=be16(p+4)*0.1-273.14;setNum(engIntake,v,"°C",1);out+=metricLine("Intake temp",v,"C",1);}
    if(have(6,2)){double v=be16(p+6)*0.1-273.14;setNum(engFuelTemp,v,"°C",1);out+=metricLine("Fuel temp",v,"C",1);}
    if(have(8,2)){double v=be16(p+8)*0.1-273.14;setNum(engOilTemp,v,"°C",1);out+=metricLine("Oil temp",v,"C",1);}
    if(have(10,2)){double v=be16(p+10)*0.1-273.14;setNum(engEgtPre,v,"°C",1);out+=metricLine("EGT before catalyst",v,"C",1);}
    if(have(12,2)){double v=be16(p+12);setNum(engRpm,v,"rpm",0);out+=metricLine("RPM",v,"rpm",0);}
    if(have(14,2)){double v=be16(p+14)*0.01;setNum(engPedal1,v,"%",2);out+=metricLine("Accelerator 1",v,"%",2);}
    if(have(16,2)){double v=be16(p+16)*0.1;setNum(engMaf,v,"kg/h",1);out+=metricLine("Air mass",v,"kg/h",1);}
    if(have(18,2)){double v=be16(p+18);setNum(engBoost,v,"hPa",0);out+=metricLine("Boost pressure",v,"hPa",0);}
    if(have(20,2)){double v=be16(p+20)*0.1;setNum(engRail,v,"bar",1);out+=metricLine("Rail pressure",v,"bar",1);}
  }

  if(id==0x13){
    if(have(2,2)){double v=be16(p+2);setNum(engAtmos,v,"hPa",0);out+=metricLine("Atmospheric pressure",v,"hPa",0);}
    if(have(4,2)){double v=be16(p+4)*0.02362;setNum(engBattery,v,"V",2);out+=metricLine("Battery voltage",v,"V",2);}
    if(have(6,2)){double v=be16(p+6)*0.0048876;setNum(engSensor5v1,v,"V",3);out+=metricLine("Sensor supply 1",v,"V",3);}
    if(have(8,2)){double v=be16(p+8)*0.0048876;setNum(engSensor5v2,v,"V",3);out+=metricLine("Sensor supply 2",v,"V",3);}
    if(have(10,2)){double v=be16(p+10)*0.01;setNum(engPedal2,v,"%",2);out+=metricLine("Accelerator 2",v,"%",2);}
    if(have(12,2)){double v=be16(p+12)*0.1-273.14;setNum(engEgtPost,v,"°C",1);out+=metricLine("EGT after catalyst",v,"C",1);}
    if(have(15,1)){double v=p[15];setNum(engOilQuality,v,"raw",0);out+=metricLine("Oil quality raw",v,"",0);}
    if(have(17,1)){double v=p[17];setNum(engOilLevel,v,"mm",0);out+=metricLine("Oil level MOK",v,"mm",0);}
    if(have(18,2)){double hpa=be16(p+18);setNum(engLowFuel,hpa/1000.0,"bar",3);out+=metricLine("Low fuel pressure",hpa/1000.0,"bar",3);}
  }

  if(id==0x18){
    if(have(2,2)){double v=be16(p+2)*0.01;setNum(engEgr,v,"%",1);out+=metricLine("EGR duty",v,"%",2);}
    if(have(4,2)){double v=be16(p+4)*0.01;setNum(engBoostDuty,v,"%",1);out+=metricLine("Boost actuator duty",v,"%",2);}
    if(have(30,2)){double v=be16(p+30)*0.1;setNum(engRail,v,"bar",1);out+=metricLine("Rail pressure",v,"bar",1);}
  }

  if(id==0x20){
    if(have(28,2)){double v=be16(p+28)*0.0048876;setNum(engLowFuelRawV,v,"V",3);out+=metricLine("Low fuel pressure sensor raw",v,"V",3);}
  }

  if(id==0x22){
    if(have(30,2)){double v=be16(p+30);setNum(engDrvCurrent,v,"mA",0);out+=metricLine("Pressure-control valve current",v,"mA",0);}
    if(have(32,2)){double hpa=be16(p+32);setNum(engLowFuelDiag,hpa/1000.0,"bar",3);out+=metricLine("Low fuel pressure diagnostic",hpa/1000.0,"bar",3);}
  }

  if(id==0x28){
    if(have(2,2))out+=metricLine("RPM",be16(p+2),"rpm",0);
    if(have(4,2)){double v=be16(p+4)*0.01;setNum(engFuelQty,v,"mm³/stroke",2);out+=metricLine("Momentary fuel amount",v,"mm3/stroke",2);}
    for(int cyl=0;cyl<5;cyl++){size_t off=6+cyl*2;if(have(off,2)){double v=be16(p+off);setNum(engCylRpm[cyl],v,"rpm",0);out+=metricLine("Cylinder "+String(cyl+1)+" selective RPM",v,"rpm",0);}}
    for(int cyl=0;cyl<5;cyl++){size_t off=18+cyl*2;if(have(off,2)){double v=sbe16(p+off)*0.01;setNum(engCylCorr[cyl],v,"mm³/stroke",2);out+=metricLine("Cylinder "+String(cyl+1)+" fuel correction",v,"mm3/stroke",2);}}
  }

  if(id==0x30){
    if(have(2,2)){double v=be16(p+2);setNum(engIdleTarget,v,"rpm",0);out+=metricLine("Idle target RPM",v,"rpm",0);}
    if(have(4,2)){double v=be16(p+4);setNum(engDiagIdleTarget,v,"rpm",0);out+=metricLine("Diagnostic idle target",v,"rpm",0);}
    if(have(6,2)){double v=be16(p+6)*0.01;setNum(engSpeedTarget,v,"km/h",2);out+=metricLine("Speed target",v,"km/h",2);}
    if(have(8,2)){double v=be16(p+8)*0.1;setNum(engAirMassTarget,v,"mg/stroke",1);out+=metricLine("Air-mass target after limiting",v,"mg/stroke",1);}
    if(have(10,2)){double v=be16(p+10)*0.1;setNum(engEgrAirMassTarget,v,"mg/stroke",1);out+=metricLine("EGR air-mass target",v,"mg/stroke",1);}
    if(have(12,2)){double v=be16(p+12);setNum(engBoostTarget,v,"hPa",0);out+=metricLine("Boost target",v,"hPa",0);}
    if(have(14,2)){double v=be16(p+14)*0.01;setNum(engFuelReqPWG,v,"mm³/stroke",2);out+=metricLine("Desired fuel quantity PWG",v,"mm3/stroke",2);}
    if(have(16,2)){double v=be16(p+16)*0.01;setNum(engFuelReqFGR,v,"mm³/stroke",2);out+=metricLine("Desired fuel quantity FGR",v,"mm3/stroke",2);}
    if(have(18,2)){double v=be16(p+18)*0.01;setNum(engFuelReqSync,v,"mm³/stroke",2);out+=metricLine("Desired fuel quantity sync",v,"mm3/stroke",2);}
    if(have(20,2)){double v=be16(p+20)*0.01;setNum(engFuelReqADR,v,"mm³/stroke",2);out+=metricLine("Desired fuel quantity ADR",v,"mm3/stroke",2);}
    if(have(22,2)){double v=be16(p+22)*0.1;setNum(engRailTarget,v,"bar",1);out+=metricLine("Rail-pressure target",v,"bar",1);}
    if(have(24,2)){double hpa=be16(p+24);setNum(engLowFuelMin,hpa/1000.0,"bar",3);out+=metricLine("Minimum low fuel pressure",hpa/1000.0,"bar",3);}
  }
  return out.length()?out:"Positive response received; no built-in decoded fields for this packet.\n";
}
bool pollLiveGroup(LiveGroup&g){uint8_t payload[]={0x21,g.id};uint8_t r[256];size_t rn=0;if(!sendKwpPayload(payload,sizeof(payload),r,rn,500)){g.raw=lastRxHex;g.decoded="No valid response: "+lastError+"\n";notePollResult(false);return false;}g.raw=bytesHex(r,rn);g.decoded=decodeLiveGroup(g.id,r,rn);notePollResult(true);return true;}
String allLiveText(){String s;for(int i=0;i<LIVE_GROUP_COUNT;i++){LiveGroup&g=liveGroups[i];if(!g.enabled&&!g.raw.length())continue;s+="=== 21 "+hex2(g.id)+" - "+String(g.name)+" ===\n";if(g.decoded.length())s+=g.decoded;else s+="Waiting for data...\n";if(g.raw.length())s+="RAW: "+g.raw+"\n";s+="\n";}return s.length()?s:"No live data yet.";}
void engineLivePollTick(){if(!livePolling||!diagnosticSession||busy)return;if((int32_t)(millis()-nextPollMs)<0)return;for(int tries=0;tries<LIVE_GROUP_COUNT;tries++){pollIndex%=LIVE_GROUP_COUNT;LiveGroup&g=liveGroups[pollIndex++];if(g.enabled){busy=true;pollLiveGroup(g);busy=false;nextPollMs=millis()+cfg.pollIntervalMs;return;}}nextPollMs=millis()+cfg.pollIntervalMs;}

String dtcCodeString(uint16_t raw){const char kinds[4]={'P','C','B','U'};char out[7];char kind=kinds[(raw>>14)&3];uint8_t d1=(raw>>12)&3;uint16_t rest=raw&0x0FFF;sprintf(out,"%c%1X%03X",kind,d1,rest);return String(out);}
String dtcStatusText(uint8_t s){uint8_t storage=(s>>5)&3;String st;if(storage==0)st="not detected";else if(storage==1)st="stored / not currently present";else if(storage==2)st="pending/implementation-specific";else st="active/stored";st+=(s&0x10)?", test incomplete":", test complete";if(s&0x80)st+=", warning requested";return st;}
bool readDTCs(){if(activeModule==MOD_ABS){dtcText="ABS DTC definitions are loaded, but the ABSBR901 CBF did not expose a verified fault-read request. No unverified command will be sent.";return false;}if(!diagnosticSession){dtcText="Not connected.";return false;}uint8_t payload[64];size_t plen=0;if(!parseHexString(cfg.dtcReadPayload,payload,plen,sizeof(payload))){dtcText="Invalid DTC read payload setting.";return false;}uint8_t r[256];size_t rn=0;busy=true;bool ok=sendKwpPayload(payload,plen,r,rn,900);busy=false;if(!ok){dtcText="DTC read failed: "+lastError+"\nRAW: "+lastRxHex;return false;}const uint8_t*p=nullptr;size_t pn=0;if(!responsePayload(r,rn,p,pn)){dtcText="Invalid KWP DTC response.";return false;}if(pn>=3&&p[0]==0x7F){dtcText="ECU negative response to service "+hex2(p[1])+": NRC "+hex2(p[2])+"\nRAW: "+bytesHex(r,rn);return false;}uint8_t expected=(uint8_t)(payload[0]+0x40);if(pn<1||p[0]!=expected){dtcText="Unexpected DTC response. Expected positive service "+hex2(expected)+".\nRAW: "+bytesHex(r,rn);return false;}if(payload[0]!=0x18){dtcText="Positive response received, but built-in DTC parser only decodes service 18.\nRAW: "+bytesHex(r,rn);return true;}if(pn<2){dtcText="Service 18 response too short.\nRAW: "+bytesHex(r,rn);return false;}uint8_t count=p[1];String out="Reported DTC count: "+String(count)+"\n";size_t pos=2;int parsed=0;while(pos+2<pn&&parsed<count){uint16_t code=((uint16_t)p[pos]<<8)|p[pos+1];uint8_t status=p[pos+2];out+=dtcCodeString(code)+"  raw="+hex2(p[pos])+hex2(p[pos+1])+"  status="+hex2(status)+"  "+dtcStatusText(status)+"\n";pos+=3;parsed++;}if(count==0)out+="No stored DTCs reported.\n";out+="RAW: "+bytesHex(r,rn);dtcText=out;return true;}
bool clearPowertrainDTCs(){if(activeModule!=MOD_ENGINE){lastError="DTC clearing is locked to the engine module in this build";return false;}if(!diagnosticSession){lastError="Not connected";return false;}uint8_t payload[64];size_t plen=0;if(!parseHexString(cfg.dtcClearPayload,payload,plen,sizeof(payload))){lastError="Invalid DTC clear payload setting";return false;}uint8_t r[256];size_t rn=0;busy=true;bool ok=sendKwpPayload(payload,plen,r,rn,900);busy=false;if(!ok)return false;const uint8_t*p=nullptr;size_t pn=0;if(!responsePayload(r,rn,p,pn))return false;uint8_t expected=(uint8_t)(payload[0]+0x40);if(pn>=1&&p[0]==expected){logx("DTC clear payload accepted by ECU");delay(200);readDTCs();return true;}lastError="Unexpected clear-DTC response: "+bytesHex(r,rn);return false;}
bool parseHexString(String s,uint8_t*out,size_t&n,size_t maxn){s.replace(","," ");s.replace("-"," ");s.trim();n=0;int p=0;while(p<(int)s.length()&&n<maxn){while(p<(int)s.length()&&s[p]==' ')p++;if(p>=(int)s.length())break;int e=s.indexOf(' ',p);if(e<0)e=s.length();String t=s.substring(p,e);char*ep;long v=strtol(t.c_str(),&ep,16);if(*ep||v<0||v>255)return false;out[n++]=(uint8_t)v;p=e+1;}return n>0;}
String sendManualPayload(String text){uint8_t p[64];size_t pn=0;if(!parseHexString(text,p,pn,sizeof(p)))return"Bad hex payload";uint8_t r[256];size_t rn=0;busy=true;bool ok=sendKwpPayload(p,pn,r,rn,900);busy=false;if(!ok)return"Failed: "+lastError+" | raw="+lastRxHex;return bytesHex(r,rn);}
