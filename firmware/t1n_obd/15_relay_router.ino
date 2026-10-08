// Four permanently wired TLIN1027-Q1 K-line transceivers.
// ESP32 UART2 is reassigned to the selected RX/TX GPIO pair; no relays.
// Each transceiver: RXD pin 1 -> RX GPIO with individual 3.3V pull-up,
// TXD pin 4 <- TX GPIO, EN pin 2 -> 3.3V, LIN pin 6 -> assigned OBD pin.
// Chip 2 original wiring: RX14 / TX27.
// Check all selected pins against the exact ESP32 board variant.
static const uint8_t K_RX[4]  = {13,14,26,33};
static const uint8_t K_TX[4]  = {23,27,25,32};
static const uint8_t K_OBD[4] = {1,7,9,11};
static const uint8_t K_MODULE[4] = {255,0,1,2}; // unassigned, engine, ABS, EGS
int selectedKline = -1;
bool relayGpioUsable(uint8_t pin){return pin>=1&&pin<=39;}
int relayIndexForModule(ModuleId m) {
  for(int i=0;i<4;i++)if(K_MODULE[i]==(uint8_t)m)return i;
  return -1;
}
void allRelaysOff() {
  // Legacy name retained for existing session/config call sites.
  // Never drive a K-line dominant as part of channel switching.
  if(uartAttached){KL.end();uartAttached=false;}
  for(int i=0;i<4;i++){pinMode(K_TX[i],OUTPUT);digitalWrite(K_TX[i],HIGH);}
}
void initRelayRouting(){
  allRelaysOff();
  selectedKline=-1;
  TX_PIN=K_TX[1];RX_PIN=K_RX[1];
  activity("Four independent TLIN1027 channels initialized (OBD 1/7/9/11); no relays");
}
bool selectRelayForModule(ModuleId m){
  int idx=relayIndexForModule(m);
  if(idx<0){lastError="No TLIN1027 channel assigned to "+moduleName(m);activity(lastError);return false;}
  allRelaysOff();
  TX_PIN=K_TX[idx];
  RX_PIN=K_RX[idx];
  selectedKline=idx;
  pinMode(RX_PIN,INPUT); // external pull-up installed on TLIN RXD
  digitalWrite(TX_PIN,HIGH); // recessive idle before any UART activity
  activity("Selected TLIN channel "+String(idx+1)+" OBD"+String(K_OBD[idx])+
           " RX GPIO"+String(RX_PIN)+" TX GPIO"+String(TX_PIN));
  return true;
}
String relayDiagnosticsJson(){
  String s="{\"activeLow\":0,\"channels\":[";
  for(int i=0;i<4;i++){
    if(i)s+=",";
    s+="{\"relay\":"+String(i+1)+",\"gpio\":"+String(K_RX[i])+
      ",\"txGpio\":"+String(K_TX[i])+",\"obdPin\":"+String(K_OBD[i])+
      ",\"module\":"+String(K_MODULE[i])+",\"commandedOn\":"+
      String(selectedKline==i?1:0)+",\"readback\":-1,\"expected\":\"UART RX/TX\"}";
  }
  return s+"]}";
}
bool manualRelayTest(int idx,String &result){
  // Old relay-test API cannot test always-wired TLIN channels.
  result="No relays installed; channel tests are unavailable. Use module Connect to test communication.";
  return false;
}
