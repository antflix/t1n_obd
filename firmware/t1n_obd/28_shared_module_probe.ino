// Read-only, allowlisted StartCommunication probe for other ECUs sharing our four K-lines.
// Never sends live-data, DTC clear, actuator, or security commands.
// Target selection is by real OBD line and ECU address, independent of legacy ModuleId.
struct SharedProbeTarget { const char *name; uint8_t obd; uint8_t address; };
static const SharedProbeTarget sharedTargets[]={
  {"Instrument Cluster / KI",15,0x69}, // Autel-confirmed
  {"HVAC / HZRKL",15,0x50},             // Autel-confirmed
  {"EWM / Shift Lever",9,0x24},         // Autel-confirmed
  {"ABS / ABSBR901",9,0x34},            // Live scanner confirmed
  {"Transmission / EGS52",11,0x20},    // Live scanner confirmed
  {"Engine / CR2",7,0x12},              // Live scanner confirmed
  {"Pin 11 candidate 0x63",11,0x63},   // Autel tried, no reply
  {"Pin 11 candidate 0x74",11,0x74},   // Autel tried, no reply
  {"Pin 9 candidate 0x32",9,0x32}      // Autel tried, no reply
};
String sharedProbeListJson(){
  String s="{\"targets\":[";
  for(size_t i=0;i<sizeof(sharedTargets)/sizeof(sharedTargets[0]);i++){
    if(i)s+=",";
    s+="{\"id\":"+String(i)+",\"name\":\""+jsonEscape(sharedTargets[i].name)+"\",\"obdPin\":"+String(sharedTargets[i].obd)+",\"address\":\"0x"+hex2(sharedTargets[i].address)+"\"}";
  }
  return s+"]}";
}
String runSharedProbe(unsigned i){
  const SharedProbeTarget &t=sharedTargets[i];
  int ch=-1;for(int k=0;k<4;k++)if(K_OBD[k]==t.obd)ch=k;
  if(ch<0)return "{\"error\":\"OBD channel not wired\"}";
  busy=true;autoReconnect=false;reconnectAtMs=0;livePolling=false;diagnosticSession=false;cr2Connected=false;
  allRelaysOff();TX_PIN=K_TX[ch];RX_PIN=K_RX[ch];pinMode(RX_PIN,INPUT_PULLUP);
  activeEcuAddr=t.address;
  releaseKL();delay(cfg.preIdleMs);
  int idle=digitalRead(RX_PIN);
  digitalWrite(TX_PIN,gpioForKLow());delayMicroseconds(25000);
  int low=digitalRead(RX_PIN);
  digitalWrite(TX_PIN,gpioForKHigh());delayMicroseconds(25000);
  int high=digitalRead(RX_PIN);
  uartOn();flushRx();
  uint8_t payload[]={0x81},tx[12],raw[256],reply[128];
  size_t n=buildKwpFrame(payload,1,tx,sizeof(tx)),fn=0;
  sendSpacedBytes(tx,n);
  size_t rn=readUntilEcuFrame(raw,sizeof(raw),reply,fn,sizeof(reply),950);
  String txHex=bytesHex(tx,n),rxHex=bytesHex(raw,rn),ecuHex=bytesHex(reply,fn);
  bool ok=false;
  if(fn){const uint8_t *p=nullptr;size_t pn=0;ok=responsePayload(reply,fn,p,pn)&&pn&&p[0]==0xC1;}
  releaseKL();allRelaysOff();activeEcuAddr=moduleAddr(activeModule);
  busy=false;setStatus("DISCONNECTED");
  activity("Shared K-line probe "+String(t.name)+" OBD"+String(t.obd)+" TX "+txHex+" RX "+rxHex+" ECU "+ecuHex);
  return "{\"target\":\""+jsonEscape(t.name)+"\",\"obdPin\":"+String(t.obd)+",\"address\":\"0x"+hex2(t.address)+"\",\"rxGpio\":"+String(K_RX[ch])+",\"txGpio\":"+String(K_TX[ch])+",\"idleHigh\":"+String(idle==HIGH?1:0)+",\"drivenLow\":"+String(low==LOW?1:0)+",\"releasedHigh\":"+String(high==HIGH?1:0)+",\"tx\":\""+txHex+"\",\"rawRx\":\""+rxHex+"\",\"ecuFrame\":\""+ecuHex+"\",\"accepted\":"+String(ok?"true":"false")+"}";
}
