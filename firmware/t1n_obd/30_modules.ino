String boolState(uint8_t raw,const char*offText,const char*onText){if(raw==0)return String(offText);if(raw==0xFF)return String(onText);return"Raw 0x"+hex2(raw);}
void markSessionLost(const String&why){diagnosticSession=false;cr2Connected=false;livePolling=false;lastError=why;releaseKL();allRelaysOff();activity("Diagnostic session lost: "+why);setStatus("SESSION LOST");if(autoReconnect)reconnectAtMs=millis()+2000;}
void notePollResult(bool ok){if(ok){consecutiveRequestFailures=0;return;}if(++consecutiveRequestFailures>=3)markSessionLost("Three consecutive diagnostic requests failed");}
String egsAsciiId(const uint8_t*p,size_t n){String s="";for(size_t i=0;i<n&&i<4;i++){uint8_t b=p[i];s+=(b>=32&&b<=126)?String((char)b):".";}return s;}
bool readEgsIdentification(){
  if(activeModule!=MOD_EGS||!diagnosticSession)return false;
  egsVariantId="";egsVariantRaw="";egsIdentRaw="";
  uint8_t r[256];size_t rn=0;const uint8_t*p=nullptr;size_t pn=0;
  // AP200 transmission_recording.sr: 1A 86 identification immediately precedes 30 01 01 variant read.
  uint8_t identReq[2]={0x1A,0x86};
  if(sendKwpPayload(identReq,sizeof(identReq),r,rn,900)&&responsePayload(r,rn,p,pn)&&pn>=2&&p[0]==0x5A&&p[1]==0x86){
    egsIdentRaw=pn>2?bytesHex(p+2,pn-2):"";
    activity("EGS ECU identification 1A 86 accepted"+(egsIdentRaw.length()?": "+egsIdentRaw:""));
  }else{
    String why=lastError.length()?lastError:"unexpected response";
    logx("EGS identification 1A 86 not accepted: "+why);
    lastError="";
  }
  uint8_t variantReq[3]={0x30,0x01,0x01};
  if(!sendKwpPayload(variantReq,sizeof(variantReq),r,rn,1000)){
    String why=lastError;activity("EGS variant read 30 01 01 failed: "+why);lastError="";return false;
  }
  if(!responsePayload(r,rn,p,pn)||pn<3||p[0]!=0x70||p[1]!=0x01||p[2]!=0x01){
    activity("EGS variant read returned unexpected response: "+bytesHex(r,rn));lastError="";return false;
  }
  egsVariantRaw=pn>3?bytesHex(p+3,pn-3):"";
  if(pn>=7)egsVariantId=egsAsciiId(p+3,4);
  activity("EGS variant identified via 30 01 01"+(egsVariantId.length()?": "+egsVariantId:"")+(egsVariantRaw.length()?" · raw "+egsVariantRaw:""));
  lastError="";return true;
}
bool connectFastModule(ModuleId module){activity("Connecting to "+moduleName(module)+" at address 0x"+hex2(moduleAddr(module))+" on OBD pin "+String(moduleObdPin(module)));busy=true;livePolling=false;diagnosticSession=false;cr2Connected=false;activeModule=module;requestedModule=module;activeEcuAddr=moduleAddr(module);consecutiveRequestFailures=0;keepaliveMisses=0;lastError="";setStatus("CONNECTING - "+moduleName(module));releaseKL();delay(cfg.preIdleMs);pinMode(TX_PIN,OUTPUT);digitalWrite(TX_PIN,gpioForKLow());delayMicroseconds(25000);digitalWrite(TX_PIN,gpioForKHigh());delayMicroseconds(25000);uartOn();flushRx();uint8_t startPayload[1]={0x81},tx[16];size_t tn=buildKwpFrame(startPayload,1,tx,sizeof(tx));sendSpacedBytes(tx,tn);uint8_t raw[256],frame[128];size_t fn=0;size_t rn=readUntilEcuFrame(raw,sizeof(raw),frame,fn,sizeof(frame),900);lastRxHex=bytesHex(raw,rn);logx(rn?"START RX RAW "+lastRxHex:"START RX RAW <nothing>");if(!rn||!fn){lastError="No valid StartCommunication response from "+moduleName(module);logx("START RX ECU <none>");activity("Connection failed: "+lastError+(rn?"; raw RX "+lastRxHex:"; no RX bytes"));setStatus("CONNECT FAILED");releaseKL();busy=false;return false;}logx("START RX ECU "+bytesHex(frame,fn));const uint8_t*rp=nullptr;size_t rpn=0;if(!responsePayload(frame,fn,rp,rpn)||rpn<1||rp[0]!=0xC1){lastError="Unexpected StartCommunication response: "+bytesHex(frame,fn);logx("START response rejected; expected positive service C1");activity("Connection failed: "+lastError);setStatus("CONNECT FAILED");releaseKL();busy=false;return false;}logx("START response accepted: positive service C1 from 0x"+hex2(moduleAddr(module)));activity("StartCommunication accepted by "+moduleName(module)+": "+bytesHex(frame,fn));lastRxHex=bytesHex(frame,fn);lastGoodTrafficMs=millis();p3ReadyAtMs=millis()+cfg.p3MinMs;diagnosticSession=true;cr2Connected=(module==MOD_ENGINE);keepaliveMisses=0;if(module==MOD_EGS)readEgsIdentification();if(module==MOD_ABS){livePolling=true;nextPollMs=millis()+80;lastError="";activity("ABS StartCommunication accepted. C1 key bytes 43 46 select LETCS framing; verified wheel-speed polling 2A 01 01 enabled.");setStatus("CONNECTED - ABS KWFB/LETCS");busy=false;return true;}activity("Connected to "+moduleName(module)+". Live polling started.");setStatus("CONNECTED - "+moduleName(module));livePolling=true;nextPollMs=millis()+80;busy=false;return true;}
bool connectModule(ModuleId module){
  if(!selectRelayForModule(module)){setStatus("K-LINE CHANNEL ERROR");return false;}
  if(module==MOD_ENGINE){bool ok=connectCR2();if(ok){activeModule=MOD_ENGINE;activeEcuAddr=0x12;requestedModule=MOD_ENGINE;livePolling=true;nextPollMs=millis()+80;}else allRelaysOff();return ok;}
  bool ok=connectFastModule(module);if(!ok)allRelaysOff();return ok;
}
bool absPayload(uint8_t group,const uint8_t*frame,size_t fn,const uint8_t*&p,size_t&pn){if(!responsePayload(frame,fn,p,pn)||pn<3)return false;return p[0]==0x6A&&p[1]==0x01&&p[2]==group;}
double absWheelSpeed(uint16_t raw){return raw<=38?0.0:raw<=4800?raw*0.0625:-1.0;}
bool pollABSGroup(uint8_t group){uint8_t payload[3]={0x2A,0x01,group},r[256];size_t rn=0;bool ok=sendKwpPayload(payload,sizeof(payload),r,rn,650);if(!ok){notePollResult(false);return false;}const uint8_t*p=nullptr;size_t pn=0;if(!absPayload(group,r,rn,p,pn)){lastError="Unexpected ABS response";notePollResult(false);return false;}if(group==1&&pn>=11){double rl=absWheelSpeed(be16(p+3)),fr=absWheelSpeed(be16(p+5)),rr=absWheelSpeed(be16(p+7)),fl=absWheelSpeed(be16(p+9));if(rl>=0)setNum(absWheelRL,rl,"km/h",1);if(fr>=0)setNum(absWheelFR,fr,"km/h",1);if(rr>=0)setNum(absWheelRR,rr,"km/h",1);if(fl>=0)setNum(absWheelFL,fl,"km/h",1);}else if(group==2&&pn>=5){setNum(absVoltage,p[3]*0.071,"V",2);setNum(absWheelSensorV,p[4]*0.0196,"V",2);}else if(group==3&&pn>=14){setTxt(absBrakeLamp,boolState(p[3],"Released","Pressed"));setTxt(absBrakeSwitch,boolState(p[4],"Pressed","Released"));setTxt(absPump,boolState(p[6],"Stopped","Running"));setTxt(absOutletFL,boolState(p[9],"Not commanded","Commanded"));setTxt(absOutletFR,boolState(p[13],"Not commanded","Commanded"));}notePollResult(true);return true;}
String egsGearName(uint8_t x){switch(x){case 0:return"N";case 1:return"1";case 2:return"2";case 3:return"3";case 4:return"4";case 5:return"5";case 11:return"R";case 12:return"R2";case 13:return"P";case 14:return"Free";case 15:return"Implausible";default:return"Raw "+String(x);}}
String egsSelectorName(uint8_t x){switch(x){case 0:return"N/A";case 1:return"1";case 2:return"2";case 3:return"3";case 4:return"4";case 5:return"D";case 6:return"N";case 7:return"R";case 8:return"P";case 9:return"+";case 10:return"-";case 11:return"N-D";case 12:return"R-N";case 13:return"P-R";case 14:return"-";case 15:return"Implausible";default:return"Raw "+String(x);}}
String egsTargetGearName(uint8_t x){switch(x){case 0x00:return"N";case 0x10:return"1";case 0x20:return"2";case 0x30:return"3";case 0x40:return"4";case 0x50:return"5";case 0xB0:return"R";case 0xC0:return"R2";case 0xD0:return"P";case 0xE0:return"Shift abort";case 0xF0:return"Implausible";default:return"Raw 0x"+hex2(x);}}
String egsRecognizedGearName(uint8_t x){switch(x){case 0:return"Inactive";case 1:return"1";case 2:return"2";case 3:return"3";case 4:return"4";case 5:return"5";case 6:return"R";case 7:return"R2";case 23:return"Wrong 3rd";case 88:return"Calculating";case 255:return"Implausible";default:return"Raw "+String(x);}}
String egsProgramName(uint8_t x){switch(x){case 0:return"S";case 1:return"W";case 2:return"A";case 3:return"M";default:return"Out of Range";}}
String egsConverterStatusName(uint8_t x){switch(x){case 0:return"Open";case 1:return"Open -> slipping";case 2:return"Slipping -> open";case 3:return"Slipping";case 4:return"Slipping -> closed";case 5:return"Closed -> slipping";case 6:return"Closed";default:return"Raw "+String(x);}}
String egsMinMaxGearName(uint8_t x){switch(x){case 0:return"Inactive";case 1:return"1";case 2:return"2";case 3:return"3";case 4:return"4";case 5:return"5";default:return"Raw "+String(x);}}
bool pollEGSGroup(uint8_t group){uint8_t payload[2]={0x21,group},r[256];size_t rn=0;bool ok=sendKwpPayload(payload,sizeof(payload),r,rn,650);if(!ok){notePollResult(false);return false;}const uint8_t*p=nullptr;size_t pn=0;if(!responsePayload(r,rn,p,pn)||pn<2||p[0]!=0x61||p[1]!=group){lastError="Unexpected EGS response";notePollResult(false);return false;}
  if(group==0x30&&pn>=24){
    setNum(egsConverterSlip,be16(p+2),"rpm",0);setNum(egsConverterTargetSlip,be16(p+4),"rpm",0);setNum(egsConverterPressure,be16(p+6),"mbar",0);setTxt(egsConverterStatus,egsConverterStatusName(p[8]));
    setTxt(egsSelector,egsSelectorName(p[9]));setTxt(egsProgram,egsProgramName(p[10]));setTxt(egsRecognizedGear,egsRecognizedGearName(p[11]));setTxt(egsGear,egsGearName(p[12]&0x0F));setTxt(egsTargetGear,egsTargetGearName(p[12]&0xF0));
    if(p[13]!=0xFF)setNum(egsTemp,(double)p[13]-50.0,"°C",0);
    setNum(egsEngineTorque,be16(p+14),"Nm",0);setNum(egsConvertedTorque,be16(p+16),"Nm",0);setNum(egsOutputRpm,be16(p+18),"rpm",0);
    setTxt(egsKickdown,(p[20]&0x01)?"Active":"Inactive");setTxt(egsDownshift,(p[22]&0x01)?"Active":"Inactive");setTxt(egsUpshift,(p[22]&0x02)?"Active":"Inactive");setTxt(egsTccActive,(p[22]&0x80)?"Active":"Inactive");
    setTxt(egsCurrentFault,(p[23]&0x01)?"Yes":"No");setTxt(egsLimp,(p[23]&0x02)?"Yes":"No");
  }else if(group==0x31&&pn>=22){
    setNum(egsN2Rpm,be16(p+2),"rpm",0);setNum(egsN3Rpm,be16(p+4),"rpm",0);setNum(egsTurbineRpm,be16(p+6),"rpm",0);setNum(egsEngineRpm,be16(p+8),"rpm",0);
    setNum(egsWheelFLRpm,be16(p+10),"rpm",0);setNum(egsWheelFRRpm,be16(p+12),"rpm",0);setNum(egsWheelRLRpm,be16(p+14),"rpm",0);setNum(egsWheelRRRpm,be16(p+16),"rpm",0);
    setNum(egsVehicleSpeed,be16(p+18)*0.1,"km/h",1);setNum(egsFrontSpeed,be16(p+20)*0.1,"km/h",1);
  }else if(group==0x32&&pn>=14){
    setNum(egsPedal,p[2],"%",0);setNum(egsGrade,be16(p+8)*0.001,"%",3);setTxt(egsMinGear,egsMinMaxGearName(p[12]));setTxt(egsMaxGear,egsMinMaxGearName(p[13]));
  }else if(group==0x33&&pn>=17){
    setNum(egsShiftPressure,be16(p+4),"mbar",0);setNum(egsModPressure,be16(p+6),"mbar",0);setNum(egsShiftCurrentTarget,be16(p+8),"mA",0);setNum(egsShiftCurrentActual,be16(p+10),"mA",0);setNum(egsModCurrentTarget,be16(p+12),"mA",0);setNum(egsModCurrentActual,be16(p+14),"mA",0);setNum(egsTccDuty,p[16],"/255",0);
  }else if(group==0x34&&pn>=14){
    setNum(egsBattery,be16(p+2)*0.025,"V",2);setNum(egsAskSupply,be16(p+4)*0.0049,"V",2);setNum(egsSensorSupply,be16(p+6)*0.0075,"V",2);setNum(egsValveSupply,be16(p+8)*0.025,"V",2);
  }else if(group==0x40&&pn>=4){
    uint16_t raw=be16(p+2);if(raw!=0xFFFF)setNum(egsOdometer,raw*2.0,"km",0);
  }else if(group==0x54&&pn>=7){
    setNum(egsCoolant,(double)p[6]-40.0,"°C",0);
  }
  notePollResult(true);return true;
}
uint8_t absPollIndex=0,egsPollIndex=0;
void absLivePollTick(){if(!livePolling||!diagnosticSession||busy||(int32_t)(millis()-nextPollMs)<0)return;busy=true;pollABSGroup(1);busy=false;nextPollMs=millis()+cfg.pollIntervalMs;}
void egsLivePollTick(){if(!livePolling||!diagnosticSession||busy||(int32_t)(millis()-nextPollMs)<0)return;/* AP200 transmission_recording.sr verified these five live-data groups. 21 40/54 remain decodable but are not in the default cycle because Autel did not request them during the complete live-data walkthrough. */static const uint8_t groups[]={0x30,0x31,0x33,0x34,0x30,0x31,0x32};busy=true;pollEGSGroup(groups[egsPollIndex++%(sizeof(groups)/sizeof(groups[0]))]);busy=false;nextPollMs=millis()+cfg.pollIntervalMs;}
void livePollTick(){if(activeModule==MOD_ENGINE)engineLivePollTick();else if(activeModule==MOD_ABS)absLivePollTick();else egsLivePollTick();}
String engineExpected(const char*id){
  String k=id;
  if(k=="rpm"){if(engIdleTarget.valid&&engSpeed.valid&&engSpeed.value<1.0)return"Idle target "+engIdleTarget.text+" rpm · warm idle usually ~680 rpm";return"Warm idle ~680 rpm · operating RPM varies";}
  if(k=="speed")return"Driving-dependent";
  if(k=="load")return"0–100% · operating-dependent";
  if(k=="coolant")return"Warm roughly 80–100 °C · investigate sustained >105 °C";
  if(k=="intake")return"Cold-soak ≈ ambient · rises with engine-bay heat/boost";
  if(k=="fuelTemp")return"Cold-soak ≈ ambient · rises while running";
  if(k=="oilTemp")return"Warm roughly 80–110 °C";
  if(k=="egtPre"||k=="egtPost")return"Load-dependent · use as a trend/comparison value";
  if(k=="maf")return"Load/EGR-dependent · sensor span roughly 15–480 kg/h";
  if(k=="pedal1"){if(engPedal2.valid)return"0–100% · pedal 2 = "+engPedal2.text+"%";return"0–100% · both pedal channels should track";}
  if(k=="pedal2"){if(engPedal1.valid)return"0–100% · pedal 1 = "+engPedal1.text+"%";return"0–100% · both pedal channels should track";}
  if(k=="atmos")return"Altitude/weather-dependent · compare with local barometric pressure";
  if(k=="battery")return engRpm.valid&&engRpm.value>400?"Running roughly 13.2–14.7 V":"Engine off: ~12.6 V full; ≥12.4 V generally healthy";
  if(k=="sensor5v1"||k=="sensor5v2")return"5 V reference · expected about 4.9–5.1 V";
  if(k=="oilQuality")return"Raw CBF presentation unresolved · display for correlation only";
  if(k=="oilLevel")return"Engine-off measurement only · sensor measuring window roughly 40–120 mm";
  if(k=="lowFuel"){if(engLowFuelMin.valid)return"ECU minimum "+engLowFuelMin.text+" bar · actual should stay ≥ minimum";return"Idle typically ~2.0–2.5 bar · ECU minimum shown separately";}
  if(k=="lowFuelRaw")return"Pressure-sensor signal roughly 0.5–3.5 V";
  if(k=="lowFuelDiag"){if(engLowFuel.valid)return"Cross-check actual "+engLowFuel.text+" bar";return"Diagnostic-converted duplicate of low-side pressure";}
  if(k=="lowFuelMin")return"ECU-calculated minimum · actual low-side pressure should stay above this";
  if(k=="boost"){if(engBoostTarget.valid){double d=engBoost.value-engBoostTarget.value;return"Target "+engBoostTarget.text+" hPa · Δ "+String(d,0)+" hPa";}return"KOEO ≈ atmospheric · under load compare with boost target";}
  if(k=="boostTarget")return"ECU target · compare with actual boost";
  if(k=="rail"){if(engRailTarget.valid){double d=engRail.value-engRailTarget.value;return"Target "+engRailTarget.text+" bar · Δ "+String(d,1)+" bar";}return"Must reach ~200 bar to start · under load compare with target";}
  if(k=="railTarget")return"ECU target · compare with actual rail pressure";
  if(k=="egr"||k=="boostDuty")return"0–100% command · operating-condition dependent";
  if(k=="drvCurrent")return"ECU-controlled · use as a trend/diagnostic value";
  if(k=="fuelQty")return"Load/RPM-dependent · mm³ per stroke";
  if(k.startsWith("cylCorr"))return"Should cluster near 0 and near the other cylinders";
  if(k.startsWith("cylRpm"))return engRpm.valid?"Should stay close to overall "+engRpm.text+" rpm and other cylinders":"Should stay close to overall RPM and other cylinders";
  if(k=="idleTarget")return"Warm idle target is usually around 680 rpm";
  if(k=="diagIdleTarget")return"Compare with ECU idle target/actual RPM when active";
  if(k=="speedTarget")return"ECU target · operating-dependent";
  if(k=="airMassTarget")return"ECU air-mass target after limiting";
  if(k=="egrAirMassTarget")return"ECU EGR-control air-mass target";
  if(k.startsWith("fuelReq"))return"ECU-requested fuel quantity · operating/control-state dependent";
  return"";
}
String engineHealth(const char*id,const ValueState&v){
  if(!v.valid||stale(v,6000))return"stale";
  String k=id;
  if(k=="coolant"){if(v.value>115)return"bad";if(v.value>105)return"warn";return"neutral";}
  if(k=="oilTemp"){if(v.value>130)return"bad";if(v.value>120)return"warn";return"neutral";}
  if(k=="battery"&&engRpm.valid&&engRpm.value>400){if(v.value<12.5||v.value>15.2)return"bad";if(v.value<13.2||v.value>14.7)return"warn";return"ok";}
  if(k=="sensor5v1"||k=="sensor5v2"){if(v.value<4.8||v.value>5.2)return"bad";if(v.value<4.9||v.value>5.1)return"warn";return"ok";}
  if(k=="lowFuel"&&engLowFuelMin.valid){if(v.value<engLowFuelMin.value)return"bad";return"ok";}
  return"neutral";
}
String valueJson(const char*id,const char*label,const ValueState&v){
  String expected=activeModule==MOD_ENGINE?engineExpected(id):"";
  String state=activeModule==MOD_ENGINE?engineHealth(id,v):(stale(v)?"stale":"neutral");
  return"{\"id\":\""+String(id)+"\",\"label\":\""+String(label)+"\",\"value\":\""+jsonEscape(v.text)+"\",\"unit\":\""+jsonEscape(v.unit)+"\",\"valid\":"+String(v.valid?1:0)+",\"stale\":"+String(stale(v)?1:0)+",\"state\":\""+state+"\",\"expected\":\""+jsonEscape(expected)+"\"}";
}
void historyObserve(const char *id,const ValueState &v);
String sensorsJson(bool captureOnly=false){
  String s=captureOnly?String(""):("{\"module\":\""+jsonEscape(moduleName(activeModule))+"\",\"items\":[");bool first=true;
  auto add=[&](const char*id,const char*label,const ValueState&v){if(captureOnly){historyObserve(id,v);return;}if(!first)s+=",";first=false;s+=valueJson(id,label,v);};
  if(activeModule==MOD_ENGINE){
    add("rpm","Engine RPM",engRpm);add("speed","Vehicle speed",engSpeed);add("coolant","Coolant temperature",engCoolant);add("oilTemp","Engine oil temperature",engOilTemp);add("battery","Battery voltage",engBattery);add("lowFuel","Low-side fuel pressure",engLowFuel);
    add("boost","Boost / MAP actual",engBoost);add("boostTarget","Boost target",engBoostTarget);add("rail","Rail pressure actual",engRail);add("railTarget","Rail pressure target",engRailTarget);
    add("load","Calculated engine load",engLoad);add("intake","Intake-air temperature",engIntake);add("fuelTemp","Fuel temperature",engFuelTemp);add("egtPre","EGT before catalyst",engEgtPre);add("egtPost","EGT after catalyst",engEgtPost);add("maf","Air mass / MAF",engMaf);
    add("pedal1","Accelerator channel 1",engPedal1);add("pedal2","Accelerator channel 2",engPedal2);add("atmos","Atmospheric pressure",engAtmos);
    add("sensor5v1","5 V sensor supply 1",engSensor5v1);add("sensor5v2","5 V sensor supply 2",engSensor5v2);add("oilQuality","Oil quality (raw)",engOilQuality);add("oilLevel","Oil level MOK",engOilLevel);
    add("lowFuelRaw","Low-side pressure sensor voltage",engLowFuelRawV);add("lowFuelDiag","Low-side pressure diagnostic",engLowFuelDiag);add("lowFuelMin","Low-side pressure ECU minimum",engLowFuelMin);
    add("egr","EGR duty",engEgr);add("boostDuty","Boost actuator duty",engBoostDuty);add("drvCurrent","Pressure-control valve current",engDrvCurrent);add("fuelQty","Momentary fuel quantity",engFuelQty);
    for(int i=0;i<5;i++){String id="cylCorr"+String(i+1),label="Cylinder "+String(i+1)+" fuel correction";add(id.c_str(),label.c_str(),engCylCorr[i]);}
    for(int i=0;i<5;i++){String id="cylRpm"+String(i+1),label="Cylinder "+String(i+1)+" selective RPM";add(id.c_str(),label.c_str(),engCylRpm[i]);}
    add("idleTarget","Idle-speed target",engIdleTarget);add("diagIdleTarget","Diagnostic idle target",engDiagIdleTarget);add("speedTarget","Vehicle-speed target",engSpeedTarget);add("airMassTarget","Air-mass target after limiting",engAirMassTarget);add("egrAirMassTarget","EGR air-mass target",engEgrAirMassTarget);
    add("fuelReqPWG","Desired fuel quantity PWG",engFuelReqPWG);add("fuelReqFGR","Desired fuel quantity FGR",engFuelReqFGR);add("fuelReqSync","Desired fuel quantity sync",engFuelReqSync);add("fuelReqADR","Desired fuel quantity ADR",engFuelReqADR);
  }else if(activeModule==MOD_ABS){
    add("wheelFL","Front-left wheel",absWheelFL);add("wheelFR","Front-right wheel",absWheelFR);add("wheelRL","Rear-left wheel",absWheelRL);add("wheelRR","Rear-right wheel",absWheelRR);add("voltage","ABS supply voltage",absVoltage);add("wheelSensorV","Wheel-sensor monitor voltage",absWheelSensorV);add("brakeLamp","Brake-light switch",absBrakeLamp);add("brakeSwitch","Brake switch",absBrakeSwitch);add("pump","Pump motor feedback",absPump);add("outletFL","Front-left outlet valve",absOutletFL);add("outletFR","Front-right outlet valve",absOutletFR);
  }else{
    add("temp","Transmission temperature",egsTemp);add("gear","Actual gear",egsGear);add("targetGear","Target gear",egsTargetGear);add("recognizedGear","Recognized gear",egsRecognizedGear);add("selector","Selector position",egsSelector);add("program","Drive program",egsProgram);add("converterStatus","Converter clutch status",egsConverterStatus);
    add("outputRpm","Output shaft RPM",egsOutputRpm);add("turbineRpm","Turbine RPM",egsTurbineRpm);add("n2","Transmission speed n2",egsN2Rpm);add("n3","Transmission speed n3",egsN3Rpm);add("engineRpm","Engine RPM received by EGS",egsEngineRpm);
    add("wheelFL","Front-left wheel RPM",egsWheelFLRpm);add("wheelFR","Front-right wheel RPM",egsWheelFRRpm);add("wheelRL","Rear-left wheel RPM",egsWheelRLRpm);add("wheelRR","Rear-right wheel RPM",egsWheelRRRpm);add("speed","Rear-wheel vehicle speed",egsVehicleSpeed);add("frontSpeed","Front-wheel vehicle speed",egsFrontSpeed);
    add("battery","TCM battery voltage",egsBattery);add("askSupply","ASK sensor supply",egsAskSupply);add("sensorSupply","Transmission sensor supply",egsSensorSupply);add("valveSupply","Valve supply",egsValveSupply);
    add("converterSlip","Converter slip actual",egsConverterSlip);add("converterTargetSlip","Converter slip target",egsConverterTargetSlip);add("converterPressure","Converter commanded pressure",egsConverterPressure);add("engineTorque","Engine torque",egsEngineTorque);add("convertedTorque","Converted engine torque",egsConvertedTorque);
    add("shiftPressure","Shift pressure",egsShiftPressure);add("modPressure","Modulating pressure",egsModPressure);add("shiftCurrentTarget","Shift regulator target current",egsShiftCurrentTarget);add("shiftCurrentActual","Shift regulator actual current",egsShiftCurrentActual);add("modCurrentTarget","Modulating regulator target current",egsModCurrentTarget);add("modCurrentActual","Modulating regulator actual current",egsModCurrentActual);add("tccDuty","Converter clutch PWM raw",egsTccDuty);
    add("pedal","Pedal value received by EGS",egsPedal);add("grade","Calculated grade",egsGrade);add("minGear","Minimum permitted gear",egsMinGear);add("maxGear","Maximum permitted gear",egsMaxGear);add("coolant","Coolant temperature received by EGS",egsCoolant);add("odometer","Odometer",egsOdometer);
    add("kickdown","Kickdown",egsKickdown);add("limp","Limp mode",egsLimp);add("currentFault","Current fault flag",egsCurrentFault);add("upshift","Upshift in progress",egsUpshift);add("downshift","Downshift in progress",egsDownshift);add("tccActive","Converter clutch active",egsTccActive);
  }
  if(!captureOnly)s+="]}";return s;
}
String moduleStatusJson(){String s="{";s+="\"firmware\":\""+String(FIRMWARE_VERSION)+"\",";s+="\"module\":\""+jsonEscape(moduleName(activeModule))+"\",";s+="\"moduleId\":"+String((int)activeModule)+",";s+="\"address\":\"0x"+hex2(activeEcuAddr)+"\",";s+="\"obdPin\":"+String(moduleObdPin(activeModule))+",";int ri=relayIndexForModule(activeModule);s+="\"relay\":"+String(ri>=0?ri+1:0)+",";s+="\"relayGpio\":"+String(ri>=0?RX_PIN:255)+",";s+="\"connected\":"+String(diagnosticSession?1:0)+",";s+="\"polling\":"+String(livePolling?1:0)+",";s+="\"busy\":"+String(busy?1:0)+",";s+="\"status\":\""+jsonEscape(scannerStatus)+"\",";s+="\"error\":\""+jsonEscape(lastError)+"\",";s+="\"variant\":\""+jsonEscape(activeModule==MOD_EGS?egsVariantId:"")+"\",";s+="\"variantRaw\":\""+jsonEscape(activeModule==MOD_EGS?egsVariantRaw:"")+"\",";s+="\"identRaw\":\""+jsonEscape(activeModule==MOD_EGS?egsIdentRaw:"")+"\",";s+="\"wifi\":\""+jsonEscape(WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():WiFi.softAPIP().toString())+"\"";s+="}";return s;}
