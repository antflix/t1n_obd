// ---------------- CR2 active tests ----------------
bool cr2TestSend(const uint8_t* payload,size_t plen,String &out){
  if(activeModule!=MOD_ENGINE){out="CR2 tests are only available on Engine / CR2.";return false;}
  if(!diagnosticSession||busy){out="Connect Engine / CR2 first.";return false;}
  uint8_t r[256];size_t rn=0;busy=true;bool ok=sendKwpPayload(payload,plen,r,rn,900);busy=false;
  if(!ok){out="Failed: "+lastError+"\nRAW: "+lastRxHex;return false;}
  const uint8_t*p=nullptr;size_t pn=0;
  if(!responsePayload(r,rn,p,pn)){out="Invalid KWP response.\nRAW: "+bytesHex(r,rn);return false;}
  if(pn>=3&&p[0]==0x7F){out="Negative response: service "+hex2(p[1])+" NRC "+hex2(p[2])+"\nRAW: "+bytesHex(r,rn);return false;}
  uint8_t expected=(uint8_t)(payload[0]+0x40);
  if(pn<1||p[0]!=expected){out="Unexpected response. Expected "+hex2(expected)+" as positive service.\nRAW: "+bytesHex(r,rn);return false;}
  out="Accepted\nRAW: "+bytesHex(r,rn);
  return true;
}
bool cr2TestSendHex(const String &hex,String &out){
  uint8_t p[64];size_t pn=0;
  if(!parseHexString(hex,p,pn,sizeof(p))){out="Bad test payload";return false;}
  return cr2TestSend(p,pn,out);
}
bool cr2TestActuator(uint8_t id,double pct,String &out){
  if(pct<0)pct=0;if(pct>100)pct=100;
  uint16_t raw=(uint16_t)lround(pct*100.0);
  uint8_t p[]={0x30,id,0x07,(uint8_t)(raw>>8),(uint8_t)raw};
  return cr2TestSend(p,sizeof(p),out);
}
bool cr2TestRelease(uint8_t id,String &out){
  uint8_t p[]={0x30,id,0x00};
  return cr2TestSend(p,sizeof(p),out);
}
