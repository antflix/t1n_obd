// Four permanently wired TLIN1027-Q1 K-line transceivers.
// ESP32 UART2 is reassigned to the selected RX/TX GPIO pair; no relays.
// Each transceiver: RXD pin 1 -> RX GPIO with individual 3.3V pull-up,
// TXD pin 4 <- TX GPIO, EN pin 2 -> 3.3V, LIN pin 6 -> assigned OBD pin.
// Temporary OBD cross-channel test: chip 1 ABS OBD9 (RX13/TX23), chip 2 EGS OBD11 (RX14/TX27).
// Chips 3 and 4 have no OBD K-line connected; engine and HVAC are not available.
// Check all selected pins against the exact ESP32 board variant.
static const uint8_t K_RX[4]  = {13,14,26,33};
static const uint8_t K_TX[4]  = {23,27,21,32};
static const uint8_t K_OBD[4] = {9,11,0,0};
static const uint8_t K_MODULE[4] = {1,2,255,255}; // ABS, EGS, disconnected, disconnected
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
  for(int i=0;i<4;i++)pinMode(K_RX[i],INPUT_PULLUP); // diagnostic fallback for TLIN1027 open-drain RXD
  selectedKline=-1;
  TX_PIN=K_TX[1];RX_PIN=K_RX[1];
  activity("Temporary channel swap: ABS=chip1/OBD9, EGS=chip2/OBD11; chips3/4 disconnected");
}
bool selectRelayForModule(ModuleId m){
  int idx=relayIndexForModule(m);
  if(idx<0){lastError="No TLIN1027 channel assigned to "+moduleName(m);activity(lastError);return false;}
  allRelaysOff();
  TX_PIN=K_TX[idx];
  RX_PIN=K_RX[idx];
  selectedKline=idx;
  pinMode(RX_PIN,INPUT_PULLUP); // TLIN1027 RXD is open drain; external 1k-10k preferred
  digitalWrite(TX_PIN,HIGH); // recessive idle before any UART activity
  activity("Selected TLIN channel "+String(idx+1)+" OBD"+String(K_OBD[idx])+
           " RX GPIO"+String(RX_PIN)+" TX GPIO"+String(TX_PIN));
  return true;
}
// Passive voltage-logic inspection: READS ONLY RXD and TXD, never drives an ECU output.
String klineLogicJson(){
  String s="{\"lines\":[";
  for(int i=0;i<4;i++){
    if(i)s+=",";
    s+="{\"obdPin\":"+String(K_OBD[i])+
       ",\"module\":\""+jsonEscape(K_MODULE[i]==255?String("Unconnected"):moduleName((ModuleId)K_MODULE[i]))+"\""+
       ",\"rxGpio\":"+String(K_RX[i])+
       ",\"txGpio\":"+String(K_TX[i])+
       ",\"rxHigh\":"+String(digitalRead(K_RX[i])==HIGH?1:0)+
       ",\"txHigh\":"+String(digitalRead(K_TX[i])==HIGH?1:0)+"}";
  }
  return s+"]}";
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
