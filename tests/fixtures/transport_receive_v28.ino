// Verbatim production functions from version28 commit 44e31d3.
// Frozen regression fixture; never included in firmware.
uint8_t checksum8(const uint8_t*b,size_t n){uint8_t s=0;for(size_t i=0;i<n;i++)s=(uint8_t)(s+b[i]);return s;}
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
