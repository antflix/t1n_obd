void sendPin15CompensatedFrame(const uint8_t *b,size_t n,int fallAdvance,int riseAdvance);
size_t readPin15CompensatedResponse(uint8_t *raw,size_t maxRaw,uint8_t *frame,size_t &frameLen,size_t maxFrame,uint32_t timeoutMs);
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

void releaseKL(){if(uartAttached){KL.end();uartAttached=false;}pinMode(TX_PIN,OUTPUT);digitalWrite(TX_PIN,gpioForKHigh());}
void uartOn(){if(uartAttached)return;pinMode(TX_PIN,OUTPUT);digitalWrite(TX_PIN,gpioForKHigh());KL.begin(cfg.baud,SERIAL_8N1,RX_PIN,TX_PIN,false);uartAttached=true;}
void flushRx(){while(KL.available())KL.read();}
void sendSpacedBytes(const uint8_t*b,size_t n){uartOn();uint32_t start=micros();for(size_t i=0;i<n;i++){if(i)waitUntilUs(start+i*cfg.byteSpacingUs);KL.write(b[i]);KL.flush();}lastTxHex=bytesHex(b,n);logx("TX "+lastTxHex);}

uint8_t checksum8(const uint8_t*b,size_t n){uint8_t s=0;for(size_t i=0;i<n;i++)s=(uint8_t)(s+b[i]);return s;}
size_t buildKwpFrame(const uint8_t*payload,size_t plen,uint8_t*out,size_t maxn){if(plen==0||plen>0x3F||maxn<plen+4)return 0;out[0]=0x80|(uint8_t)plen;out[1]=activeEcuAddr;out[2]=cfg.testerAddr;memcpy(out+3,payload,plen);out[3+plen]=checksum8(out,3+plen);return plen+4;}
bool validChecksum(const uint8_t*f,size_t n){return n>=5&&checksum8(f,n-1)==f[n-1];}
bool extractEcuFrame(const uint8_t*raw,size_t rn,uint8_t*frame,size_t&fn,size_t maxn){for(size_t i=0;i+4<rn;i++){uint8_t fmt=raw[i];if((fmt&0xC0)!=0x80)continue;size_t plen=fmt&0x3F,total=plen+4;if(plen==0||i+total>rn||total>maxn)continue;if(raw[i+1]!=cfg.testerAddr||raw[i+2]!=activeEcuAddr)continue;if(!validChecksum(raw+i,total))continue;memcpy(frame,raw+i,total);fn=total;return true;}return false;}
// HTTP is served by the main loop, outside UART receive and timing windows.
size_t readRaw(uint8_t*out,size_t maxn,uint32_t firstTimeoutMs,uint32_t quietMs){size_t n=0;uint32_t start=millis(),last=start;bool any=false;while(millis()-start<firstTimeoutMs){while(KL.available()){uint8_t b=(uint8_t)KL.read();if(n<maxn)out[n++]=b;any=true;last=millis();}if(any&&millis()-last>=quietMs)break;delay(1);}return n;}
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
    if(!gotByte) delay(1);
  }
  if(n) extractEcuFrame(out,n,frame,fn,frameMax);
  return n;
}
void waitP3Ready(){while((int32_t)(p3ReadyAtMs-millis())>0){delay(1);}}
bool sendKwpPayload(const uint8_t*payload,size_t plen,uint8_t*response,size_t&responseLen,uint32_t timeoutMs=0){responseLen=0;if(!cr2Connected&&!diagnosticSession){lastError="Not connected";return false;}if(!payload||plen==0){lastError="Empty payload";return false;}uint8_t tx[80];size_t tn=buildKwpFrame(payload,plen,tx,sizeof(tx));if(!tn){lastError="Bad KWP frame";return false;}if(activeModule==MOD_ABS && plen+5<=sizeof(tx)){memmove(tx+4,tx+3,plen);tx[0]=0x80;tx[3]=(uint8_t)plen;tx[plen+4]=checksum8(tx,plen+4);tn=plen+5;}if(!timeoutMs)timeoutMs=cfg.rxFirstTimeoutMs;waitP3Ready();uint8_t raw[512];size_t rn=0;if(moduleObdPin(activeModule)==15){sendPin15CompensatedFrame(tx,tn,11,75);rn=readPin15CompensatedResponse(raw,sizeof(raw),response,responseLen,256,timeoutMs);}else{uartOn();flushRx();sendSpacedBytes(tx,tn);rn=readUntilEcuFrame(raw,sizeof(raw),response,responseLen,256,timeoutMs);}lastRxHex=bytesHex(raw,rn);logx(rn?"RX RAW "+lastRxHex:"RX RAW <nothing>");if(!rn||!responseLen){lastError=rn?"No ECU frame (echo/noise)":"No ECU response";return false;}lastRxHex=bytesHex(response,responseLen);logx("RX ECU "+lastRxHex);lastGoodTrafficMs=millis();p3ReadyAtMs=millis()+cfg.p3MinMs;keepaliveMisses=0;lastError="";return true;}
bool responsePayload(const uint8_t*frame,size_t fn,const uint8_t*&p,size_t&pn){if(fn<5)return false;pn=frame[0]&0x3F;if(pn+4!=fn)return false;p=frame+3;return true;}
bool responsePayloadStartsWith(const uint8_t*frame,size_t fn,const String&expectedHex){if(!expectedHex.length())return true;uint8_t exp[64];size_t en=0;if(!parseHexString(expectedHex,exp,en,sizeof(exp)))return false;const uint8_t*p=nullptr;size_t pn=0;if(!responsePayload(frame,fn,p,pn)||pn<en)return false;for(size_t i=0;i<en;i++)if(p[i]!=exp[i])return false;return true;}

void waitMs(uint32_t ms){uint32_t until=millis()+ms;while((int32_t)(until-millis())>0){delay(1);}}
bool sendPostC1Request(){if(!cfg.sendPostC1){logx("Post-C1 request disabled; CR2 remains open from C1");return true;}uint8_t payload[64];size_t pn=0;if(!parseHexString(cfg.postC1Payload,payload,pn,sizeof(payload))){lastError="Invalid post-C1 payload setting";return !cfg.requirePostC1;}logx("Waiting post-C1 delay "+String(cfg.postC1DelayMs)+" ms before "+cfg.postC1Payload);waitMs(cfg.postC1DelayMs);uint8_t r[256];size_t rn=0;bool ok=sendKwpPayload(payload,pn,r,rn,cfg.postC1TimeoutMs);if(!ok){String why="Post-C1 request got no valid ECU response: "+lastError;logx(why);lastError=why;return !cfg.requirePostC1;}if(!responsePayloadStartsWith(r,rn,cfg.postC1Expected)){String why="Post-C1 response did not match expected payload prefix "+cfg.postC1Expected+": "+bytesHex(r,rn);logx(why);lastError=why;return !cfg.requirePostC1;}logx("Post-C1 response accepted: "+bytesHex(r,rn));lastError="";return true;}

bool testerPresent(){if(!diagnosticSession)return false;uint8_t pld[64];size_t pn=0;if(!parseHexString(activeModule==MOD_EWM?String("83 00"):cfg.testerPresentPayload,pld,pn,sizeof(pld))){lastError="Invalid TesterPresent payload setting";return false;}uint8_t r[256];size_t rn=0;bool ok=sendKwpPayload(pld,pn,r,rn,cfg.rxFirstTimeoutMs);if(ok)ok=responsePayloadStartsWith(r,rn,activeModule==MOD_EWM?String("C3"):cfg.testerPresentExpected);if(ok){keepaliveMisses=0;return true;}keepaliveMisses++;logx("TesterPresent missed ("+String(keepaliveMisses)+"/"+String(cfg.keepaliveMissLimit)+")");if(keepaliveMisses>=cfg.keepaliveMissLimit)markSessionLost("TesterPresent failed repeatedly");return false;}
void disconnectCurrentModule(){activity("Disconnecting from "+moduleName(activeModule));if(activeModule==MOD_HZR&&hzrFanOverrideActive&&diagnosticSession){String fanStop;bool stopped=hzrFanStop(fanStop);activity(String(stopped?"HVAC fan control released: ":"WARNING - HVAC fan release unconfirmed: ")+fanStop);}livePolling=false;if(diagnosticSession&&cfg.disconnectPayload.length()){uint8_t pld[64];size_t pn=0;if(parseHexString(cfg.disconnectPayload,pld,pn,sizeof(pld))){uint8_t r[256];size_t rn=0;sendKwpPayload(pld,pn,r,rn,350);}}diagnosticSession=false;cr2Connected=false;releaseKL();allRelaysOff();activity("Disconnected from "+moduleName(activeModule)+"; all K-line transceivers idle");setStatus("DISCONNECTED");}
