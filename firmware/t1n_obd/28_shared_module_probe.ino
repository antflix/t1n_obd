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


// Timing experiment isolated to physical channel 2, OBD15. The ESP32 hardware
// UART remains untouched for all normal module connections and other channels.
// Byte waveform is 8N1 at 10400 baud; LOW/HIGH transitions are pre-positioned
// to counteract observed asymmetric TLIN/bus latency. Logic analyzer on TXD/RXD
// is the reference. Diagnostic probe is StartCommunication only, no controls.
void sendPin15CompensatedByte(uint8_t value,uint32_t t0,int fallAdvance,int riseAdvance){
  const uint32_t bitUs=1000000UL/cfg.baud;
  // bit 0=start (LOW), bits 1..8=data LSB first, bit 9=stop (HIGH).
  bool bits[10];bits[0]=false;
  for(int i=1;i<=8;i++)bits[i]=((value>>(i-1))&1)!=0;
  bits[9]=true;
  for(int i=0;i<10;){
    if(bits[i]){i++;continue;}
    int end=i+1;while(end<10&&!bits[end])end++;
    // The rising edge is early by ~65us more than the falling edge.
    waitUntilUs(t0+(uint32_t)i*bitUs-(uint32_t)fallAdvance);
    digitalWrite(TX_PIN,gpioForKLow());
    waitUntilUs(t0+(uint32_t)end*bitUs-(uint32_t)riseAdvance);
    digitalWrite(TX_PIN,gpioForKHigh());
    i=end;
  }
  waitUntilUs(t0+10*bitUs);
}
String runPin15TimingTest(int fallAdvance,int riseAdvance,uint8_t address){
  // Constrain experimental timings to avoid invalid bit-cell transition ordering.
  if(fallAdvance<0||fallAdvance>30||riseAdvance<0||riseAdvance>85||
     riseAdvance-fallAdvance>85)
    return "{\"error\":\"Invalid timing parameters\"}";
  if(K_OBD[1]!=15)return "{\"error\":\"Chip 2 is not on OBD15\"}";
  busy=true;autoReconnect=false;reconnectAtMs=0;
  livePolling=false;diagnosticSession=false;cr2Connected=false;
  allRelaysOff();TX_PIN=K_TX[1];RX_PIN=K_RX[1];pinMode(RX_PIN,INPUT_PULLUP);
  releaseKL();delay(cfg.preIdleMs);
  // Standard KWP fast-init wakeup; leave UART disconnected so GPIO owns TXD.
  digitalWrite(TX_PIN,gpioForKLow());delayMicroseconds(25000);
  digitalWrite(TX_PIN,gpioForKHigh());delayMicroseconds(25000);
  uint8_t tx[5]={0x81,address,cfg.testerAddr,0x81,0};
  tx[4]=checksum8(tx,4);
  for(int i=0;i<5;i++){
    uint32_t t0=micros()+200;
    noInterrupts();
    sendPin15CompensatedByte(tx[i],t0,fallAdvance,riseAdvance);
    interrupts();
    // Keep byte spacing near current 5952us baseline; the 200us preamble
    // is constant and UART RXD is observed externally by the analyzer.
    if(i<4)delayMicroseconds(cfg.byteSpacingUs-1000);
  }
  digitalWrite(TX_PIN,gpioForKHigh());
  delay(1100);releaseKL();allRelaysOff();
  busy=false;setStatus("DISCONNECTED");
  activity("Pin15 timing probe 0x"+hex2(address)+" fallAdvance="+String(fallAdvance)+
    " riseAdvance="+String(riseAdvance)+" TX "+bytesHex(tx,5));
  return "{\"obdPin\":15,\"address\":\"0x"+hex2(address)+"\",\"fallAdvanceUs\":"+
    String(fallAdvance)+",\"riseAdvanceUs\":"+String(riseAdvance)+
    ",\"tx\":\""+bytesHex(tx,5)+"\",\"capture\":\"D0 RXD, D1 TXD\"}";
}
