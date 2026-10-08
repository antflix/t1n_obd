// ---------------- Helpers ----------------
int gpioForKLow(){return TX_DRIVE_LOW_HIGH?HIGH:LOW;}
int gpioForKHigh(){return TX_DRIVE_LOW_HIGH?LOW:HIGH;}
String hex2(uint8_t b){char x[3];sprintf(x,"%02X",b);return String(x);}
String bytesHex(const uint8_t*b,size_t n){String s;for(size_t i=0;i<n;i++){if(i)s+=' ';s+=hex2(b[i]);}return s;}
bool parseHexString(String s,uint8_t*out,size_t&n,size_t maxn);
void applyPollGroups();
String jsonEscape(String s){String o;for(size_t i=0;i<s.length();i++){char c=s[i];if(c==92){o+=char(92);o+=char(92);}else if(c==34){o+=char(92);o+=char(34);}else if(c==10){o+=char(92);o+='n';}else if(c!=13)o+=c;}return o;}
String uptimeText(){uint32_t ms=millis();uint32_t sec=ms/1000;uint32_t min=sec/60;sec%=60;uint32_t hr=min/60;min%=60;char b[24];sprintf(b,"T+%02lu:%02lu:%02lu",(unsigned long)hr,(unsigned long)min,(unsigned long)sec);return String(b);}
void activity(const String&s){String line="["+uptimeText()+"] "+s;activityLogText+=line+"\n";if(activityLogText.length()>50000)activityLogText.remove(0,12000);Serial.println("ACT "+line);}
void logx(const String&s){String line="["+String(millis())+"ms] "+s;Serial.println(line);logText+=line+"\n";if(logText.length()>60000)logText.remove(0,15000);}
void setStatus(const String&s){scannerStatus=s;logx("STATUS: "+s);}
void waitUntilUs(uint32_t target){while((int32_t)(target-micros())>0){int32_t left=(int32_t)(target-micros());if(left>2000)delayMicroseconds(400);}}
void waitReplayUs(uint32_t off){waitUntilUs(replayT0+off);}

void releaseKL(){if(uartAttached){KL.end();uartAttached=false;}pinMode(TX_PIN,OUTPUT);digitalWrite(TX_PIN,gpioForKHigh());}
void uartOn(){if(uartAttached)return;pinMode(TX_PIN,OUTPUT);digitalWrite(TX_PIN,gpioForKHigh());KL.begin(cfg.baud,SERIAL_8N1,RX_PIN,TX_PIN,false);uartAttached=true;}
void flushRx(){while(KL.available())KL.read();}
void sendSpacedBytes(const uint8_t*b,size_t n){uartOn();uint32_t start=micros();for(size_t i=0;i<n;i++){if(i)waitUntilUs(start+i*cfg.byteSpacingUs);KL.write(b[i]);KL.flush();}lastTxHex=bytesHex(b,n);logx("TX "+lastTxHex);}

uint8_t checksum8(const uint8_t*b,size_t n){uint8_t s=0;for(size_t i=0;i<n;i++)s=(uint8_t)(s+b[i]);return s;}
size_t buildKwpFrame(const uint8_t*payload,size_t plen,uint8_t*out,size_t maxn){if(plen==0||plen>0x3F||maxn<plen+4)return 0;out[0]=0x80|(uint8_t)plen;out[1]=activeEcuAddr;out[2]=cfg.testerAddr;memcpy(out+3,payload,plen);out[3+plen]=checksum8(out,3+plen);return plen+4;}
bool validChecksum(const uint8_t*f,size_t n){return n>=5&&checksum8(f,n-1)==f[n-1];}
bool extractEcuFrame(const uint8_t*raw,size_t rn,uint8_t*frame,size_t&fn,size_t maxn){for(size_t i=0;i+4<rn;i++){uint8_t fmt=raw[i];if((fmt&0xC0)!=0x80)continue;size_t plen=fmt&0x3F,total=plen+4;if(plen==0||i+total>rn||total>maxn)continue;if(raw[i+1]!=cfg.testerAddr||raw[i+2]!=activeEcuAddr)continue;if(!validChecksum(raw+i,total))continue;memcpy(frame,raw+i,total);fn=total;return true;}return false;}
size_t readRaw(uint8_t*out,size_t maxn,uint32_t firstTimeoutMs,uint32_t quietMs){size_t n=0;uint32_t start=millis(),last=start;bool any=false;while(millis()-start<firstTimeoutMs){while(KL.available()){uint8_t b=(uint8_t)KL.read();if(n<maxn)out[n++]=b;any=true;last=millis();}if(any&&millis()-last>=quietMs)break;web.handleClient();delay(1);}return n;}
size_t readUntilEcuFrame(uint8_t*out,size_t maxn,uint8_t*frame,size_t&fn,size_t frameMax,uint32_t timeoutMs){
  size_t n=0; fn=0; uint32_t start=millis();
  while(millis()-start<timeoutMs){
    bool gotByte=false;
    while(KL.available()){
      uint8_t b=(uint8_t)KL.read();
      if(n<maxn) out[n++]=b;
      gotByte=true;
    }
    if(n&&extractEcuFrame(out,n,frame,fn,frameMax)) return n;
    web.handleClient();
    if(!gotByte) delay(1);
  }
  if(n) extractEcuFrame(out,n,frame,fn,frameMax);
  return n;
}
void waitP3Ready(){while((int32_t)(p3ReadyAtMs-millis())>0){web.handleClient();delay(1);}}
bool sendKwpPayload(const uint8_t*payload,size_t plen,uint8_t*response,size_t&responseLen,uint32_t timeoutMs=0){responseLen=0;if(!cr2Connected&&!diagnosticSession){lastError="Diagnostic link is not open";return false;}if(!payload||plen==0){lastError="Empty KWP payload";return false;}uint8_t tx[80];size_t tn=buildKwpFrame(payload,plen,tx,sizeof(tx));if(!tn){lastError="Could not build KWP frame";return false;}if(!timeoutMs)timeoutMs=cfg.rxFirstTimeoutMs;waitP3Ready();uartOn();flushRx();sendSpacedBytes(tx,tn);uint8_t raw[512];size_t rn=readUntilEcuFrame(raw,sizeof(raw),response,responseLen,256,timeoutMs);lastRxHex=bytesHex(raw,rn);logx(rn?"RX RAW "+lastRxHex:"RX RAW <nothing>");if(!rn||!responseLen){lastError=rn?"No valid ECU KWP frame found before timeout (echo/noise only)":"No ECU response";return false;}lastRxHex=bytesHex(response,responseLen);logx("RX ECU "+lastRxHex);lastGoodTrafficMs=millis();p3ReadyAtMs=millis()+cfg.p3MinMs;keepaliveMisses=0;lastError="";return true;}
bool responsePayload(const uint8_t*frame,size_t fn,const uint8_t*&p,size_t&pn){if(fn<5)return false;pn=frame[0]&0x3F;if(pn+4!=fn)return false;p=frame+3;return true;}
bool responsePayloadStartsWith(const uint8_t*frame,size_t fn,const String&expectedHex){if(!expectedHex.length())return true;uint8_t exp[64];size_t en=0;if(!parseHexString(expectedHex,exp,en,sizeof(exp)))return false;const uint8_t*p=nullptr;size_t pn=0;if(!responsePayload(frame,fn,p,pn)||pn<en)return false;for(size_t i=0;i<en;i++)if(p[i]!=exp[i])return false;return true;}

void collectReplayRx(){if(!uartAttached)return;while(KL.available()){int x=KL.read();if(x>=0&&eventCount<EVENT_MAX){events[eventCount].us=micros()-replayT0;events[eventCount].b=(uint8_t)x;eventCount++;}}}
void waitReplayCollect(uint32_t off){while((int32_t)(off-(micros()-replayT0))>0){collectReplayRx();if((int32_t)(off-(micros()-replayT0))>1500)delayMicroseconds(300);web.handleClient();}collectReplayRx();}
void timedSendAt(uint32_t atUs,const uint8_t*b,size_t n){waitReplayCollect(atUs);uartOn();collectReplayRx();uint32_t local=micros();for(size_t i=0;i<n;i++){while((int32_t)((local+i*cfg.byteSpacingUs)-micros())>0)collectReplayRx();KL.write(b[i]);KL.flush();collectReplayRx();}}
int lastFastRxDuringLow=-1,lastFastRxAfterRelease=-1;
void timedFastAt(uint32_t wakeUs,const uint8_t*b,size_t n){waitReplayCollect(wakeUs);if(uartAttached){collectReplayRx();KL.end();uartAttached=false;}pinMode(TX_PIN,OUTPUT);digitalWrite(TX_PIN,gpioForKLow());uint32_t lowEnd=micros()+cfg.fastLowUs;delayMicroseconds(200);lastFastRxDuringLow=digitalRead(RX_PIN);while((int32_t)(lowEnd-micros())>0){}digitalWrite(TX_PIN,gpioForKHigh());delayMicroseconds(200);lastFastRxAfterRelease=digitalRead(RX_PIN);uint32_t firstAt=replayT0+wakeUs+cfg.fastFirstByteUs;while((int32_t)(firstAt-micros())>2500){}uartOn();collectReplayRx();while((int32_t)(firstAt-micros())>0)collectReplayRx();uint32_t local=micros();for(size_t i=0;i<n;i++){while((int32_t)((local+i*cfg.byteSpacingUs)-micros())>0)collectReplayRx();KL.write(b[i]);KL.flush();collectReplayRx();}}
void timedSlowAt(uint32_t atUs,uint8_t v){waitReplayCollect(atUs);if(uartAttached){collectReplayRx();KL.end();uartAttached=false;}pinMode(TX_PIN,OUTPUT);digitalWrite(TX_PIN,gpioForKLow());delayMicroseconds(cfg.slowBitUs);for(int i=0;i<8;i++){digitalWrite(TX_PIN,(v&(1<<i))?gpioForKHigh():gpioForKLow());delayMicroseconds(cfg.slowBitUs);}digitalWrite(TX_PIN,gpioForKHigh());delayMicroseconds(cfg.slowBitUs);uartOn();collectReplayRx();}
bool replayContains(const uint8_t*pat,size_t pn){if(!pn||eventCount<pn)return false;for(size_t i=0;i+pn<=eventCount;i++){bool ok=true;for(size_t j=0;j<pn;j++)if(events[i+j].b!=pat[j]){ok=false;break;}if(ok)return true;}return false;}
bool waitReplayPattern(const uint8_t*pat,size_t pn,uint32_t maxAfterNowUs){uint32_t until=micros()+maxAfterNowUs;while((int32_t)(until-micros())>0){collectReplayRx();if(replayContains(pat,pn))return true;web.handleClient();delayMicroseconds(250);}collectReplayRx();return replayContains(pat,pn);}
void logInitTx(const char*kind,uint32_t atUs,const uint8_t*b,size_t n){
  logx("INIT TX @"+String(atUs)+"us "+String(kind)+" "+bytesHex(b,n));
}
void logCr2InitTrace(const uint8_t*c133,size_t nC,const uint8_t*f7,size_t nF,const uint8_t*vin,size_t nV,const uint8_t*a81,size_t n81,const uint8_t*a82,size_t n82,const uint8_t*cr2,size_t nCR){
  logx("----- CR2 INIT TRACE -----");
  logx("K-line RX observation on final fast init: driven LOW="+String(lastFastRxDuringLow==LOW?"LOW":"HIGH")+", released HIGH="+String(lastFastRxAfterRelease==HIGH?"HIGH":"LOW"));
  logInitTx("FAST",cfg.rC133_1,c133,nC);
  logInitTx("FAST",cfg.rC133_2,c133,nC);
  uint8_t slow[1]={cfg.slowAddress}; logInitTx("5-BAUD",cfg.rSlow,slow,1);
  logInitTx("UART",cfg.rF7,f7,nF);
  logInitTx("UART",cfg.rVin1,vin,nV);
  logInitTx("UART",cfg.rVin2,vin,nV);
  logInitTx("UART",cfg.rVin3,vin,nV);
  logInitTx("FAST",cfg.rA81_1,a81,n81);
  logInitTx("FAST",cfg.rA81_2,a81,n81);
  logInitTx("FAST",cfg.rA81_3,a81,n81);
  logInitTx("UART",cfg.rA82_1,a82,n82);
  logInitTx("UART",cfg.rA82_2,a82,n82);
  logInitTx("UART",cfg.rA82_3,a82,n82);
  logInitTx("FAST",cfg.rCR2,cr2,nCR);
  if(!eventCount){
    logx("INIT RX <nothing captured>");
  }else{
    logx("INIT RX captured "+String(eventCount)+" byte events");
    String line;
    for(size_t i=0;i<eventCount;i++){
      String item=String(events[i].us)+"us:"+hex2(events[i].b);
      if(line.length()+item.length()+1>180){logx("INIT RX "+line);line="";}
      if(line.length())line+=" ";
      line+=item;
    }
    if(line.length())logx("INIT RX "+line);
  }
  logx("----- END CR2 INIT TRACE -----");
}

void waitMsWeb(uint32_t ms){uint32_t until=millis()+ms;while((int32_t)(until-millis())>0){web.handleClient();delay(1);}}
bool sendPostC1Request(){if(!cfg.sendPostC1){logx("Post-C1 request disabled; CR2 remains open from C1");return true;}uint8_t payload[64];size_t pn=0;if(!parseHexString(cfg.postC1Payload,payload,pn,sizeof(payload))){lastError="Invalid post-C1 payload setting";return !cfg.requirePostC1;}logx("Waiting post-C1 delay "+String(cfg.postC1DelayMs)+" ms before "+cfg.postC1Payload);waitMsWeb(cfg.postC1DelayMs);uint8_t r[256];size_t rn=0;bool ok=sendKwpPayload(payload,pn,r,rn,cfg.postC1TimeoutMs);if(!ok){String why="Post-C1 request got no valid ECU response: "+lastError;logx(why);lastError=why;return !cfg.requirePostC1;}if(!responsePayloadStartsWith(r,rn,cfg.postC1Expected)){String why="Post-C1 response did not match expected payload prefix "+cfg.postC1Expected+": "+bytesHex(r,rn);logx(why);lastError=why;return !cfg.requirePostC1;}logx("Post-C1 response accepted: "+bytesHex(r,rn));lastError="";return true;}

bool connectCR2(){activity("Connecting to Engine / CR2 at address 0x12 on OBD pin 7");activeModule=MOD_ENGINE;activeEcuAddr=0x12;requestedModule=MOD_ENGINE;busy=true;livePolling=false;cr2Connected=false;diagnosticSession=false;keepaliveMisses=0;lastError="";setStatus("CONNECTING - AUTEL PREAMBLE");uint8_t c133[64],f7[64],vin[64],a81[64],a82[64],cr2[64],sync[32],comp[32],c1[64];size_t nC=0,nF=0,nV=0,n81=0,n82=0,nCR=0,nSync=0,nComp=0,nC1=0;bool parsed=parseHexString(cfg.frameC133,c133,nC,sizeof(c133))&&parseHexString(cfg.frameF7,f7,nF,sizeof(f7))&&parseHexString(cfg.frameVIN,vin,nV,sizeof(vin))&&parseHexString(cfg.frameA81,a81,n81,sizeof(a81))&&parseHexString(cfg.frameA82,a82,n82,sizeof(a82))&&parseHexString(cfg.frameCR2,cr2,nCR,sizeof(cr2))&&parseHexString(cfg.expectedSync,sync,nSync,sizeof(sync))&&parseHexString(cfg.expectedComp,comp,nComp,sizeof(comp))&&parseHexString(cfg.expectedC1,c1,nC1,sizeof(c1));if(!parsed){lastError="Invalid hex in replay frame or expected-response settings";activity("CR2 connection failed: "+lastError);setStatus("CONFIG ERROR");busy=false;return false;}eventCount=0;releaseKL();delay(cfg.preIdleMs);uartOn();flushRx();replayT0=micros();timedFastAt(cfg.rC133_1,c133,nC);timedFastAt(cfg.rC133_2,c133,nC);timedSlowAt(cfg.rSlow,cfg.slowAddress);timedSendAt(cfg.rF7,f7,nF);timedSendAt(cfg.rVin1,vin,nV);timedSendAt(cfg.rVin2,vin,nV);timedSendAt(cfg.rVin3,vin,nV);timedFastAt(cfg.rA81_1,a81,n81);timedFastAt(cfg.rA81_2,a81,n81);timedFastAt(cfg.rA81_3,a81,n81);timedSendAt(cfg.rA82_1,a82,n82);timedSendAt(cfg.rA82_2,a82,n82);timedSendAt(cfg.rA82_3,a82,n82);timedFastAt(cfg.rCR2,cr2,nCR);bool gotC1=waitReplayPattern(c1,nC1,cfg.c1WaitMs*1000UL);bool gotSync=replayContains(sync,nSync),gotComp=replayContains(comp,nComp);logCr2InitTrace(c133,nC,f7,nF,vin,nV,a81,n81,a82,n82,cr2,nCR);logx(String("Init check: ")+cfg.expectedSync+"="+(gotSync?"YES":"NO")+", "+cfg.expectedComp+"="+(gotComp?"YES":"NO")+", CR2 C1="+(gotC1?"YES":"NO"));activity(String("CR2 init result: sync ")+(gotSync?"YES":"NO")+", complement "+(gotComp?"YES":"NO")+", C1 "+(gotC1?"YES":"NO")+", captured RX bytes "+String(eventCount));if(!gotC1){lastError="CR2 StartCommunication response not found";activity("CR2 connection failed: "+lastError);setStatus("CONNECT FAILED");releaseKL();busy=false;return false;}cr2Connected=true;diagnosticSession=true;lastGoodTrafficMs=millis();p3ReadyAtMs=millis()+cfg.p3MinMs;setStatus("CR2 OPEN - POST-C1");if(!sendPostC1Request()&&cfg.requirePostC1){diagnosticSession=false;setStatus("CR2 OPEN - REQUIRED POST-C1 FAILED");busy=false;return false;}cr2Connected=true;diagnosticSession=true;lastGoodTrafficMs=millis();activity("Connected to Engine / CR2. Diagnostic session open.");setStatus("CONNECTED - CR2 READY");busy=false;return true;}
bool testerPresent(){if(!diagnosticSession)return false;uint8_t pld[64];size_t pn=0;if(!parseHexString(cfg.testerPresentPayload,pld,pn,sizeof(pld))){lastError="Invalid TesterPresent payload setting";return false;}uint8_t r[256];size_t rn=0;bool ok=sendKwpPayload(pld,pn,r,rn,cfg.rxFirstTimeoutMs);if(ok)ok=responsePayloadStartsWith(r,rn,cfg.testerPresentExpected);if(ok){keepaliveMisses=0;return true;}keepaliveMisses++;logx("TesterPresent missed ("+String(keepaliveMisses)+"/"+String(cfg.keepaliveMissLimit)+")");if(keepaliveMisses>=cfg.keepaliveMissLimit)markSessionLost("TesterPresent failed repeatedly");return false;}
void disconnectCurrentModule(){activity("Disconnecting from "+moduleName(activeModule));livePolling=false;if(diagnosticSession&&cfg.disconnectPayload.length()){uint8_t pld[64];size_t pn=0;if(parseHexString(cfg.disconnectPayload,pld,pn,sizeof(pld))){uint8_t r[256];size_t rn=0;sendKwpPayload(pld,pn,r,rn,350);}}diagnosticSession=false;cr2Connected=false;releaseKL();allRelaysOff();activity("Disconnected from "+moduleName(activeModule)+"; all K-line transceivers idle");setStatus("DISCONNECTED");}
