// ---------------- CR2 active tests ----------------
String kwpNrcText(uint8_t nrc){switch(nrc){case 0x10:return"General reject";case 0x11:return"Service not supported";case 0x12:return"Sub-function not supported";case 0x21:return"Busy / repeat request";case 0x22:return"Conditions not correct";case 0x31:return"Request out of range";case 0x33:return"Security access denied";case 0x35:return"Invalid key";case 0x36:return"Exceeded attempts";case 0x37:return"Required delay not expired";case 0x78:return"Response pending";default:return"Unknown NRC 0x"+hex2(nrc);}}
String cr2TestLabel(const String&n){if(n=="epc_on")return"EPC diagnostic lamp ON";if(n=="epc_off")return"EPC diagnostic lamp OFF";if(n=="glow_lamp_on")return"Glow-plug indicator ON";if(n=="glow_lamp_off")return"Glow-plug indicator OFF";if(n=="heater_on")return"Auxiliary heater ON";if(n=="heater_off")return"Auxiliary heater OFF";if(n=="fan_on")return"Fan output ON";if(n=="fan_off")return"Fan output OFF";if(n=="ac_cut_on")return"A/C cutoff ON";if(n=="ac_cut_off")return"A/C cutoff OFF";if(n=="ac_can_on")return"A/C CAN output ON";if(n=="ac_can_off")return"A/C CAN output OFF";if(n=="compression_lrr_on")return"Compression test start (smooth-running controller active)";if(n=="compression_lrr_off")return"Compression test start (smooth-running controller disabled)";if(n=="compression_stop")return"Compression test stop";return n;}
bool cr2TestSend(const uint8_t* payload,size_t plen,String &out){
  if(activeModule!=MOD_ENGINE){out="CR2 tests are only available on Engine / CR2.";return false;}
  if(!diagnosticSession||busy){out="Connect Engine / CR2 first.";return false;}
  uint8_t r[256];size_t rn=0;busy=true;bool ok=sendKwpPayload(payload,plen,r,rn,900);busy=false;
  if(!ok){out="Failed: "+lastError+"\nRAW: "+lastRxHex;return false;}
  const uint8_t*p=nullptr;size_t pn=0;
  if(!responsePayload(r,rn,p,pn)){out="Invalid KWP response.\nRAW: "+bytesHex(r,rn);return false;}
  if(pn>=3&&p[0]==0x7F){out="ECU rejected request\nService 0x"+hex2(p[1])+" · NRC 0x"+hex2(p[2])+" ("+kwpNrcText(p[2])+")\nRAW: "+bytesHex(r,rn);activity("CR2 test rejected: service 0x"+hex2(p[1])+", NRC 0x"+hex2(p[2])+" ("+kwpNrcText(p[2])+")");return false;}
  uint8_t expected=(uint8_t)(payload[0]+0x40);
  if(pn<1||p[0]!=expected){out="Unexpected response. Expected "+hex2(expected)+" as positive service.\nRAW: "+bytesHex(r,rn);return false;}
  out="ECU accepted command\nPositive KWP response received. This confirms command acceptance, not physical actuator movement.\nRAW: "+bytesHex(r,rn);activity("CR2 test command accepted by ECU: "+bytesHex(payload,plen));
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
