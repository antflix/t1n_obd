// ---------------- K-line relay routing ----------------
// Common 4-channel optocoupler relay boards are active LOW.
// Only one relay is ever energized at a time.
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
    uint8_t pin=cfg.relayGpio[i];
    if(!relayGpioUsable(pin))continue;
    pinMode(pin,OUTPUT);
    digitalWrite(pin,relayOffLevel());
  }
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
  delay(15);
  activity("Relay K"+String(idx+1)+" ON: GPIO"+String(pin)+" -> OBD pin "+String(cfg.relayObdPin[idx])+" -> "+moduleName(m));
  return true;
}
