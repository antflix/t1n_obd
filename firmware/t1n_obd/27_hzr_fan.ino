// Minimal HZRKL condenser fan diagnostic test, OBD15, address 0x50.
// CBF: FN_IO_Komponenten / E_LuefterKuehlmodul_VR6.
// Fan activation payload is an UNVERIFIED candidate; the FF mask can change other HVAC outputs.
bool hzrFanOverrideActive = false;
static uint32_t hzrFanStopAtMs = 0;
static String hzrFanLastFeedback = "No fan test has been run";

bool hzrRequest(const uint8_t *cmd, size_t len, uint8_t expected, String &out,
                uint8_t *fanBit = nullptr) {
  if (activeModule != MOD_HZR || !diagnosticSession || busy) {
    out = "Connect HVAC / HZRKL (OBD15) first; scanner must be idle.";
    return false;
  }
  uint8_t response[256]; size_t responseLen=0;
  busy = true;
  bool sent = sendKwpPayload(cmd,len,response,responseLen,950);
  busy = false;
  if (!sent) { out="No confirmed reply: "+lastError+" / "+lastRxHex; return false; }
  const uint8_t *p=nullptr; size_t n=0;
  if (!responsePayload(response,responseLen,p,n)) {
    out = "Invalid KWP reply: "+bytesHex(response,responseLen);
    return false;
  }
  if (n>=3 && p[0]==0x7F) {
    out = "ECU rejected 0x"+hex2(p[1])+" with NRC 0x"+hex2(p[2])+
          " ("+kwpNrcText(p[2])+")";
    return false;
  }
  if (!n || p[0]!=expected) {
    out = "Unexpected response "+bytesHex(p,n)+
          "; expected service 0x"+hex2(expected);
    return false;
  }
  if (fanBit) {
    if (n<3 || p[1]!=0x64) {out="Response did not contain 21 64 status: "+bytesHex(p,n);return false;}
    *fanBit = (p[2] & 0x04) ? 1 : 0;
  }
  out = "Positive ECU response: "+bytesHex(p,n);
  return true;
}

bool hzrFanRead(String &out) {
  const uint8_t req[]={0x21,0x64};
  uint8_t active=0;
  if (!hzrRequest(req,sizeof(req),0x61,out,&active)) return false;
  out = String("Auxiliary fan commanded ") + (active?"ON":"OFF") +
        " (HZRKL 21 64, bit 2). This is ECU status, not proof that the motor spins.\n" + out;
  activity("HZRKL fan status: "+String(active?"ON":"OFF"));
  return true;
}

bool hzrFanStop(String &out) {
  const uint8_t req[]={0x30,0x10,0x00}; // FN_IO_Beenden
  bool ok=hzrRequest(req,sizeof(req),0x70,out);
  hzrFanOverrideActive=false;
  hzrFanStopAtMs=0;
  if(ok) {
    hzrFanLastFeedback = "HZRKL actuator control released. "+out;
  }else{
    hzrFanLastFeedback = "WARNING: Could not confirm HZRKL release: "+out+
       ". If fan continues, turn ignition OFF and verify normal operation.";
  }
  activity(hzrFanLastFeedback);
  return ok;
}

bool hzrFanStart(String &out) {
  if (hzrFanOverrideActive) {
    out="Fan test already running; automatic release is pending.";
    return false;
  }
  // The CBF describes this IO-control service in all 13 HVAC variants,
  // but the fan option is marked VR6. Do not claim successful physical movement.
  const uint8_t req[]={0x30,0x10,0x07,0xFF,0x40};
  String response;
  bool ok=hzrRequest(req,sizeof(req),0x70,response);
  if (!ok) {
    out="Fan request was not confirmed: "+response;
    // ECU might have received the request even if its reply was lost. Try release.
    if (diagnosticSession && activeModule==MOD_HZR) {
      String stop;
      bool released=hzrFanStop(stop);
      out+=released?"\nRelease acknowledged.":"\nWARNING: release not confirmed. "+stop;
    }
    hzrFanLastFeedback=out;
    return false;
  }
  hzrFanOverrideActive=true;
  hzrFanStopAtMs=millis()+4000; // bounded trial, not an OEM-prescribed duration
  hzrFanLastFeedback="Fan command accepted; 4-second automatic release pending. "+response;
  activity(hzrFanLastFeedback);
  out=hzrFanLastFeedback+"\nMay temporarily override other HVAC outputs; not a physical fan confirmation.";
  return true;
}

void hzrFanTick() {
  if (!hzrFanOverrideActive || (int32_t)(millis()-hzrFanStopAtMs)<0 || busy) return;
  if (activeModule==MOD_HZR && diagnosticSession) {
    String result;
    hzrFanStop(result);
  } else {
    hzrFanOverrideActive=false;
    hzrFanLastFeedback="WARNING: Fan test expired without a connection to send release. Turn ignition OFF if the fan continues.";
    activity(hzrFanLastFeedback);
  }
}

String hzrFanFeedbackJson() {
  return "{\"active\":"+String(hzrFanOverrideActive?1:0)+
    ",\"message\":\""+jsonEscape(hzrFanLastFeedback)+"\"}";
}
