// ---------------- K-line relay routing ----------------
// Common 4-channel optocoupler relay boards are active LOW.
// Only one relay is ever energized at a time.
bool relayCommandedOn[4]={false,false,false,false};
String relayLevelText(int v){return v==LOW?"LOW (~0V expected)":"HIGH (~3.3V expected)";}
String relaySnapshot(){
  String s;
  for(int i=0;i<4;i++){
    if(i)s+=" | ";
    uint8_t pin=cfg.relayGpio[i];
    int rb=relayGpioUsable(pin)?digitalRead(pin):-1;
    s+="K"+String(i+1)+" GPIO"+String(pin)+" cmd="+String(relayCommandedOn[i]?"ON":"OFF")+" readback="+(rb<0?String("INVALID"):relayLevelText(rb));
  }
  return s;
}
bool relayGpioUsable(uint8_t pin){
  switch(pin){
    case 0: case 2: case 4: case 5:
    case 12: case 13: case 14: case 15:
    case 18: case 19: case 21: case 22: case 23:
    case 25: case 26: case 27: case 32: case 33:
      return true;
    default:return false;
  }
}
int relayOffLevel(){return cfg.relayActiveLow?HIGH:LOW;}
int relayOnLevel(){return cfg.relayActiveLow?LOW:HIGH;}
void allRelaysOff(){
  for(int i=0;i<4;i++){
    relayCommandedOn[i]=false;
    uint8_t pin=cfg.relayGpio[i];
    if(!relayGpioUsable(pin))continue;
    pinMode(pin,OUTPUT);
    digitalWrite(pin,relayOffLevel());
  }
  delay(2);
  activity("Relay outputs set OFF: "+relaySnapshot());
}
void initRelayRouting(){
  allRelaysOff();
  activity("Relay router initialized: K1 GPIO"+String(cfg.relayGpio[0])+"->OBD"+String(cfg.relayObdPin[0])+
           ", K2 GPIO"+String(cfg.relayGpio[1])+"->OBD"+String(cfg.relayObdPin[1])+
           ", K3 GPIO"+String(cfg.relayGpio[2])+"->OBD"+String(cfg.relayObdPin[2])+
           ", K4 GPIO"+String(cfg.relayGpio[3])+"->OBD"+String(cfg.relayObdPin[3]));
}
int relayIndexForModule(ModuleId m){
  for(int i=0;i<4;i++)if(cfg.relayModule[i]==(uint8_t)m)return i;
  return -1;
}
bool selectRelayForModule(ModuleId m){
  releaseKL();
  allRelaysOff();
  delay(25);
  int idx=relayIndexForModule(m);
  if(idx<0){
    lastError="No relay channel assigned to "+moduleName(m);
    activity("Relay routing failed: "+lastError);
    return false;
  }
  uint8_t pin=cfg.relayGpio[idx];
  if(!relayGpioUsable(pin)){
    lastError="Invalid relay GPIO "+String(pin)+" for "+moduleName(m);
    activity("Relay routing failed: "+lastError);
    return false;
  }
  digitalWrite(pin,relayOnLevel());
  relayCommandedOn[idx]=true;
  delay(15);
  int rb=digitalRead(pin);
  activity("Relay K"+String(idx+1)+" command ON: GPIO"+String(pin)+" written "+String(relayOnLevel()==LOW?"LOW":"HIGH")+
           ", readback "+relayLevelText(rb)+", path OBD pin "+String(cfg.relayObdPin[idx])+" -> "+moduleName(m));
  activity("Relay snapshot after select: "+relaySnapshot());
  return true;
}

String relayDiagnosticsJson(){
  String s="{\"activeLow\":"+String(cfg.relayActiveLow?1:0)+",\"channels\":[";
  for(int i=0;i<4;i++){
    if(i)s+=",";
    uint8_t pin=cfg.relayGpio[i];
    int rb=relayGpioUsable(pin)?digitalRead(pin):-1;
    s+="{\"relay\":"+String(i+1)+",\"gpio\":"+String(pin)+",\"obdPin\":"+String(cfg.relayObdPin[i])+
       ",\"module\":"+String(cfg.relayModule[i])+",\"commandedOn\":"+String(relayCommandedOn[i]?1:0)+
       ",\"readback\":"+String(rb)+",\"expected\":\""+String(relayCommandedOn[i]?(relayOnLevel()==LOW?"LOW (~0V)":"HIGH (~3.3V)"):(relayOffLevel()==LOW?"LOW (~0V)":"HIGH (~3.3V)"))+"\"}";
  }
  s+="]}";
  return s;
}
bool manualRelayTest(int idx,String &result){
  if(busy||jobConnect||diagnosticSession){result="Disconnect scanner before manual relay testing.";return false;}
  releaseKL();
  allRelaysOff();
  if(idx<0){result="All relays OFF\n"+relaySnapshot();activity("Manual relay test: all OFF");return true;}
  if(idx>3){result="Invalid relay index";return false;}
  uint8_t pin=cfg.relayGpio[idx];
  if(!relayGpioUsable(pin)){result="Invalid GPIO for K"+String(idx+1);return false;}
  digitalWrite(pin,relayOnLevel());
  relayCommandedOn[idx]=true;
  delay(20);
  int rb=digitalRead(pin);
  result="K"+String(idx+1)+" commanded ON\nGPIO "+String(pin)+" written "+String(relayOnLevel()==LOW?"LOW":"HIGH")+
         "\nDigital readback: "+relayLevelText(rb)+"\nOBD pin "+String(cfg.relayObdPin[idx]);
  activity("Manual relay test K"+String(idx+1)+": "+result);
  return rb==relayOnLevel();
}
