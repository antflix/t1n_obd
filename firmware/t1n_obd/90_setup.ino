#if __has_include("wifi_build_config.h")
#include "wifi_build_config.h"
#endif
uint32_t lastDtcScanMs=0,lastWifiRetryMs=0;
String wifiConfigSource="none";
// Event callback runs on the Wi-Fi task: only write plain scalar fields here.
volatile int wifiEventType=0,wifiReason=-1;
volatile uint32_t wifiLastEventMs=0;
uint32_t wifiConnectAttempts=0;
void wifiEventLog(WiFiEvent_t event,WiFiEventInfo_t info){
 wifiLastEventMs=millis();
 if(event==ARDUINO_EVENT_WIFI_STA_CONNECTED){wifiEventType=1;wifiReason=-1;}
 else if(event==ARDUINO_EVENT_WIFI_STA_GOT_IP){wifiEventType=2;wifiReason=-1;}
 else if(event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED){wifiReason=info.wifi_sta_disconnected.reason;wifiEventType=3;}
}
String wifiLastEventText(){
 switch(wifiEventType){
 case 1:return "Associated with router; awaiting IP";
 case 2:return "Connected and IP acquired";
 case 3:return "Station disconnected";
 default:return "No station event received";
 }
}
String wifiDisconnectExplanation(){
 switch(wifiReason){
 case 2:return "Authentication expired (router did not complete authentication)";
 case 15:return "Four-way WPA handshake timed out";
 case 201:return "No matching access point found";
 case 202:return "Router rejected authentication";
 case 203:return "Association failed";
 case 204:return "Connection/handshake timed out";
 case -1:return "No disconnect reason recorded";
 default:return "Wi-Fi reason "+String(wifiReason);
 }
}
String wifiStatusName(wl_status_t st){
  switch(st){case WL_CONNECTED:return "Connected";case WL_NO_SSID_AVAIL:return "Network not found";case WL_CONNECT_FAILED:return "Authentication/connection failed";case WL_CONNECTION_LOST:return "Connection lost";case WL_DISCONNECTED:return "Disconnected";case WL_IDLE_STATUS:return "Connecting/idle";default:return "Status "+String((int)st);}
}
void applyWifiSettings(){
#ifdef T1N_BUILD_WIFI_PASSWORD
  wifiSSID="AntNet";
  wifiPASS=T1N_BUILD_WIFI_PASSWORD;
  wifiConfigSource="GitHub build secret";
#else
  wifiConfigSource=wifiSSID.length()?"legacy saved settings":"not configured";
#endif
  Preferences wp;
  if(wp.begin("t1nwifi",true)){
    String overrideSSID=wp.getString("ssid","");
    if(overrideSSID.length()){wifiSSID=overrideSSID;wifiPASS=wp.getString("password","");wifiConfigSource="Web UI saved settings";}
    wp.end();
  }
}
String wifiDiagnosticsJson(){
  String s="{";
  s+="\"ssid\":\""+jsonEscape(wifiSSID)+"\",";
  s+="\"source\":\""+jsonEscape(wifiConfigSource)+"\",";
  s+="\"status\":\""+wifiStatusName(WiFi.status())+"\",";
  s+="\"statusCode\":"+String((int)WiFi.status())+",";
  s+="\"stationIP\":\""+WiFi.localIP().toString()+"\",";
  s+="\"apIP\":\""+WiFi.softAPIP().toString()+"\",";
  s+="\"rssi\":"+String(WiFi.status()==WL_CONNECTED?WiFi.RSSI():0)+",";
  s+="\"passwordConfigured\":"+String(wifiPASS.length()?"true":"false")+",";
  s+="\"passwordLength\":"+String(wifiPASS.length())+",";s+="\"lastEvent\":\""+jsonEscape(wifiLastEventText())+"\",";s+="\"disconnectReason\":"+String(wifiReason)+",";s+="\"disconnectExplanation\":\""+jsonEscape(wifiDisconnectExplanation())+"\",";s+="\"connectAttempts\":"+String(wifiConnectAttempts)+",";s+="\"eventAgeSeconds\":"+String((millis()-wifiLastEventMs)/1000);
  return s+"}";
}
String wifiPage(){
  return R"UI(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"><title>T1N Wi-Fi</title><style>body{font:16px system-ui;background:#091423;color:#f2f6ff;max-width:620px;margin:auto;padding:22px}section{background:#152539;border:1px solid #33506e;border-radius:15px;padding:18px;margin-top:15px}input,button{font:inherit;padding:12px;margin:7px 0;border-radius:9px}input{width:100%;box-sizing:border-box}button{background:#2779e6;color:#fff;border:0}a{color:#9fcaff}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style></head><body><a href="/">Back to scanner</a><h2>Van Wi-Fi connection</h2><section><pre id="status">Checking...</pre><button onclick="refresh()">Refresh status</button><button onclick="scan()">Scan nearby networks</button><pre id="scanOutput"></pre></section><section><h3>Change Wi-Fi credentials</h3><p>Only use if the built-in AntNet connection fails. Saved settings override the GitHub build credentials.</p><form action="/wifi/save" method="POST"><label>Wi-Fi network<input name="ssid" id="ssid" required maxlength="32"></label><label>Wi-Fi password<input name="password" type="password" required minlength="8" maxlength="63"></label><button type="submit">Save and reconnect</button></form></section><script>async function scan(){let p=document.querySelector("#scanOutput");p.textContent="Scanning...";try{let d=await(await fetch("/api/wifi/scan")).json();p.textContent=d.error?d.error:(d.networks.map(x=>x.ssid+" / "+x.rssi+" dBm / channel "+x.channel).join(String.fromCharCode(10))||"Scan completed: 0 networks");}catch(e){p.textContent="Wi-Fi scan error: "+String(e)}}async function refresh(){try{let d=await(await fetch('/api/wifi',{cache:'no-store'})).json();document.querySelector('#status').textContent='Network: '+d.ssid+'\nSource: '+d.source+'\nWi-Fi: '+d.status+' (code '+d.statusCode+')\nStation IP: '+d.stationIP+'\nSignal: '+d.rssi+' dBm\nPassword configured: '+d.passwordConfigured+' ('+d.passwordLength+' characters)\nAccess point: '+d.apIP+'\nLast event: '+d.lastEvent+'\nDisconnect reason: '+d.disconnectReason+' ('+d.disconnectExplanation+')'+'\nConnection attempts: '+d.connectAttempts+'\nEvent age: '+d.eventAgeSeconds+' seconds';document.querySelector('#ssid').value=d.ssid||'';}catch(e){document.querySelector('#status').textContent='Unable to retrieve status: '+e}}refresh();setInterval(refresh,4000)</script></body></html>)UI";
}

#ifdef T1N_USE_GZIP_PAGE
#include "scanner_ui_gzip_generated.h"
#include "advanced_ui_gzip_generated.h"
#endif

bool rejectAutomaticOwner(){
  if(!monEnabled)return false;
  web.sendHeader("Cache-Control","no-store");
  web.send(409,"application/json","{\"ok\":false,\"code\":\"AUTOMATIC_UART_OWNER\",\"error\":\"Pause automatic monitoring before manual diagnostics\"}");
  return true;
}
static bool firmwareUploadAllowed=false,firmwareUploadComplete=false;
void setupRoutes(){
web.on("/api/alerts",HTTP_GET,alertsSend);
web.on("/api/alerts/config",HTTP_POST,alertsConfigure);
web.on("/api/alerts/ack",HTTP_POST,alertsAcknowledge);
web.on("/api/alerts/events",HTTP_GET,[]{monitorEventsSend("/t1nlogs/alert_events.csv");});
web.on("/api/monitor/dtc",HTTP_GET,monitorDtcSend);
web.on("/api/monitor/dtc/events",HTTP_GET,[]{monitorEventsSend("/t1nlogs/dtc_events.csv");});
web.on("/api/sd/status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",sdStatusJson());});
web.on("/api/sd/test",HTTP_POST,[]{String msg;bool ok=sdTestCard(msg);web.send(ok?200:409,"text/plain",msg);});
web.on("/api/sd/upload",HTTP_POST,sdUploadFinish,sdUploadChunk);
web.on("/api/sd/file",HTTP_GET,sdServeAssetDownload);
web.on("/api/monitor/status",HTTP_GET,[]{
  web.sendHeader("Cache-Control","no-store");
  web.send(200,"application/json",monitorStatusJson());
});
web.on("/api/monitor/clock",HTTP_GET,[]{
  web.sendHeader("Cache-Control","no-store");
  web.send(200,"application/json",historyClockJson());
});
web.on("/api/monitor/selftest",HTTP_POST,[]{
  String result;
  bool ok=monitorStorageSelfTest(result);
  web.send(ok?200:409,"text/plain",result);
});
web.on("/api/monitor/dtc/selftest",HTTP_POST,[]{
  String result;
  bool ok=monitorDtcSelfTest(result);
  web.send(ok?200:500,"text/plain",result);
});
web.on("/api/monitor/enable",HTTP_POST,[]{
  String enabled=web.arg("enabled");
  if(enabled!="0"&&enabled!="1"){web.send(400,"text/plain","Set enabled=0 or 1");return;}
  if(busy||jobConnect){web.send(409,"text/plain","Diagnostic link busy");return;}
  monitorSetEnabled(enabled=="1");
  web.send(200,"application/json",monitorStatusJson());
});
web.on("/api/hardware/lines",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",klineLogicJson());});
web.on("/api/modules",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",modulesCatalogJson());});
web.on("/api/module/groups",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",moduleGroupsJson());});
web.on("/api/module/read",HTTP_POST,[]{if(rejectAutomaticOwner())return;
  if(!diagnosticSession||busy||jobConnect){web.send(409,"text/plain","Connect the desired module, then stop live polling before probing.");return;}
  if(activeModule==MOD_HZR||activeModule==MOD_KI||activeModule==MOD_EWM){if(livePolling){web.send(409,"text/plain","Stop live polling before individual group requests");return;}busy=true;bool ok=false;String data=readSharedGroupByRequest(web.arg("request"),ok);busy=false;web.send(ok?200:422,"text/plain; charset=utf-8",data);return;}
  String arg=web.arg("group");if(arg.length()<1||arg.length()>2){web.send(400,"text/plain","ABS: 01-04; EGS: 30-38,40,54.");return;}
  char* endPtr=nullptr;unsigned long id=strtoul(arg.c_str(),&endPtr,16);
  if(!endPtr||*endPtr!=0||id>255){web.send(400,"text/plain","Invalid hex group");return;}
  if((activeModule==MOD_ABS&&(id<1||id>(absKm20?4:3)))||(activeModule==MOD_EGS&&egsGroupIndex((uint8_t)id)<0)
    ||(activeModule!=MOD_ABS&&activeModule!=MOD_EGS)){web.send(400,"text/plain","Unsupported group");return;}
  busy=true;bool ok=false;String result=probeKnownModuleGroup((uint8_t)id,ok);busy=false;
  web.send(ok?200:422,"text/plain; charset=utf-8",result);
});
web.on("/wifi",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"text/html; charset=utf-8",wifiPage());});
web.on("/api/wifi",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",wifiDiagnosticsJson());});
web.on("/api/wifi/scan",HTTP_GET,[]{
 if(monEnabled||busy||jobConnect||diagnosticSession){web.send(409,"application/json","{\"error\":\"Pause diagnostics before Wi-Fi radio scan\"}");return;}
 // Scan results must distinguish a radio/busy error from genuinely zero networks.
 int n=WiFi.scanNetworks(false,true);
 if(n<0){WiFi.scanDelete();web.send(503,"application/json","{\"error\":\"Scan failed or station radio is busy; try again after reconnect settles\"}");return;}
 String s="{\"networks\":[";
 for(int i=0;i<n;i++){if(i)s+=",";s+="{\"ssid\":\""+jsonEscape(WiFi.SSID(i))+"\",\"rssi\":"+String(WiFi.RSSI(i))+",\"channel\":"+String(WiFi.channel(i))+"}";}
 s+="]}";WiFi.scanDelete();web.send(200,"application/json",s);
});
web.on("/wifi/save",HTTP_POST,[]{
  String ssid=web.arg("ssid"),password=web.arg("password");ssid.trim();
  if(!ssid.length()||ssid.length()>32||password.length()<8||password.length()>63){web.send(400,"text/plain","Invalid Wi-Fi network or password length");return;}
  Preferences wp;if(!wp.begin("t1nwifi",false)){web.send(500,"text/plain","Unable to save Wi-Fi settings");return;}
  wp.putString("ssid",ssid);wp.putString("password",password);wp.end();
  wifiSSID=ssid;wifiPASS=password;wifiConfigSource="Web UI saved settings";lastWifiRetryMs=millis();
  web.sendHeader("Location","/wifi",true);web.send(303,"text/plain","Saved; reconnecting");
  WiFi.disconnect();wifiConnectAttempts++;WiFi.begin(wifiSSID.c_str(),wifiPASS.c_str());
});
web.on("/pi",HTTP_GET,[]{if(sdServePage("/www/pi.html"))return;web.send(503,"text/plain","Pi dashboard unavailable on SD card");});
web.on("/",HTTP_GET,[]{
  if(sdServePage("/www/index.html"))return;
  web.sendHeader("Cache-Control","no-store");
#ifdef T1N_USE_GZIP_PAGE
  web.sendHeader("Content-Encoding","gzip");
  web.send_P(200,"text/html; charset=utf-8",(const char*)SCANNER_UI_GZIP,SCANNER_UI_GZIP_LEN);
#else
  web.send(200,"text/html; charset=utf-8",scannerPage());
#endif
});
web.on("/advanced",HTTP_GET,[]{if(sdServePage("/www/advanced.html"))return;web.sendHeader("Cache-Control","no-store");
#ifdef T1N_USE_GZIP_PAGE
  web.sendHeader("Content-Encoding","gzip");
  web.send_P(200,"text/html; charset=utf-8",(const char*)ADVANCED_UI_GZIP,ADVANCED_UI_GZIP_LEN);
#else
  web.send(200,"text/html; charset=utf-8",advancedPage());
#endif
});
web.on("/update",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store, no-cache, must-revalidate");web.sendHeader("Pragma","no-cache");web.send(200,"text/html; charset=utf-8",updatePage());});
web.on("/api/status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",moduleStatusJson());});
web.on("/api/sensors",HTTP_GET,[]{sensorsJson();});
web.on("/api/history/status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",historyStatusJson());});
web.on("/api/history/data",HTTP_GET,[]{
  String mod=web.arg("module"),id=web.arg("id"),range=web.arg("range");
  if(mod.length()!=1||mod[0]<'0'||mod[0]>'5'||!id.length()||id.length()>50
     ||(range!="1h"&&range!="24h"&&range!="7d"&&range!="30d"&&range!="all")){
    web.send(400,"application/json","{\"ok\":false,\"code\":\"INVALID_HISTORY_QUERY\",\"error\":\"Invalid module, metric or range\",\"points\":[]}");return;
  }
  historyDataSend((uint8_t)(mod[0]-'0'),id,range,
    strtoul(web.arg("before").c_str(),nullptr,10),strtoul(web.arg("serial").c_str(),nullptr,10),
    (uint8_t)web.arg("tier").toInt());
});
web.on("/api/history/clock",HTTP_POST,[]{
  String epoch=web.arg("epoch");
  if(epoch.length()<10||epoch.length()>12){web.send(400,"text/plain","Invalid UTC time");return;}
  char *end=nullptr;unsigned long value=strtoul(epoch.c_str(),&end,10);
  if(!end||*end||!historySetClock((uint32_t)value)){web.send(400,"text/plain","Invalid UTC time");return;}
  web.send(200,"text/plain","Clock synchronized");
});

web.on("/sensors",HTTP_GET,[]{sensorsJson();});
web.on("/api/dtc",HTTP_GET,[]{web.send(200,"text/plain",dtcText);});
web.on("/api/activity-log",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"text/plain; charset=utf-8",activityLogText);});
web.on("/api/activity-log.txt",HTTP_GET,[]{web.sendHeader("Content-Disposition","attachment; filename=t1n_scanner_activity_log.txt");web.send(200,"text/plain; charset=utf-8",activityLogText);});
web.on("/api/activity-log/clear",HTTP_POST,[]{activityLogText="";activity("Activity log cleared");web.send(200,"text/plain","Cleared");});
web.on("/api/relay/status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",relayDiagnosticsJson());});
web.on("/api/relay/test",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect||diagnosticSession){web.send(409,"text/plain","Disconnect scanner before manual relay testing.");return;}String a=web.arg("relay");int idx=a=="off"?-1:a.toInt()-1;String out;bool ok=manualRelayTest(idx,out);web.send(ok?200:400,"text/plain",out);});
web.on("/api/shared-probe/targets",HTTP_GET,[]{web.send(200,"application/json",sharedProbeListJson());});
web.on("/api/shared-probe/run",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect||diagnosticSession){web.send(409,"text/plain","Disconnect active scanner first");return;}if(!web.hasArg("id")){web.send(400,"text/plain","Missing target id");return;}int id=web.arg("id").toInt();if(id<0||id>=(int)(sizeof(sharedTargets)/sizeof(sharedTargets[0]))){web.send(400,"text/plain","Target not allowlisted");return;}web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",runSharedProbe((unsigned)id));});
#ifdef T1N_ENABLE_PIN15_TIMING_LAB
web.on("/api/shared-probe/timing",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect||diagnosticSession){web.send(409,"text/plain","Disconnect scanner first");return;}int fa=web.hasArg("fall")?web.arg("fall").toInt():11;int ra=web.hasArg("rise")?web.arg("rise").toInt():75;int addr=web.hasArg("address")?strtol(web.arg("address").c_str(),nullptr,0):0x50;if((addr!=0x50&&addr!=0x69)||fa<0||fa>30||ra<0||ra>85){web.send(400,"text/plain","Invalid timing or address");return;}web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",runPin15TimingTest(fa,ra,(uint8_t)addr));});
#endif
web.on("/api/module/select",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect){web.send(409,"text/plain","Scanner is busy");return;}int id=web.arg("id").toInt();if(id<0||id>5){web.send(400,"text/plain","Unknown module");return;}autoReconnect=false;reconnectAtMs=0;if(diagnosticSession)disconnectCurrentModule();requestedModule=(ModuleId)id;activeModule=requestedModule;activeEcuAddr=moduleAddr(activeModule);dtcText=activeModule==MOD_ABS?"ABS DTC read request is not verified yet.":"No DTC read yet.";activity("User selected "+moduleName(activeModule)+"; waiting for Connect");setStatus("DISCONNECTED - "+moduleName(activeModule)+" SELECTED");web.send(200,"text/plain","Module selected");});
web.on("/api/module/connect",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect){web.send(409,"text/plain","Scanner is busy");return;}int id=web.arg("id").toInt();if(id<0||id>5){web.send(400,"text/plain","Unknown module");return;}autoReconnect=true;requestedModule=(ModuleId)id;activity("User requested connection to "+moduleName(requestedModule));if(diagnosticSession)disconnectCurrentModule();activeModule=requestedModule;activeEcuAddr=moduleAddr(activeModule);dtcText=activeModule==MOD_ABS?"ABS DTC read request is not verified yet.":"No DTC read yet.";setStatus("CONNECT QUEUED - "+moduleName(activeModule));jobConnect=true;web.send(202,"text/plain","Connection queued");});
web.on("/api/hzr/fan/status",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(activeModule!=MOD_HZR||!diagnosticSession||busy){web.send(409,"text/plain","Connect HVAC / HZRKL first");return;}String out;bool ok=hzrFanRead(out);web.send(ok?200:500,"text/plain",out);});
 web.on("/api/hzr/fan/on",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(activeModule!=MOD_HZR||!diagnosticSession||busy){web.send(409,"text/plain","Connect HVAC / HZRKL first");return;}String out;bool ok=hzrFanStart(out);web.send(ok?200:500,"text/plain",out);});
 web.on("/api/hzr/fan/state",HTTP_GET,[]{web.send(200,"application/json",hzrFanFeedbackJson());});
 web.on("/api/disconnect",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy){web.send(409,"text/plain","Busy");return;}activity("User requested disconnect");autoReconnect=false;reconnectAtMs=0;disconnectCurrentModule();web.send(200,"text/plain","Disconnected");});
web.on("/api/poll/start",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(!diagnosticSession){web.send(409,"text/plain","Connect the selected module first");return;}livePolling=true;nextPollMs=millis();activity("Live data polling started for "+moduleName(activeModule));web.send(200,"text/plain","Live data started");});
web.on("/api/poll/stop",HTTP_POST,[]{if(rejectAutomaticOwner())return;livePolling=false;activity("Live data polling stopped for "+moduleName(activeModule));web.send(200,"text/plain","Live data stopped");});
web.on("/api/dtc/read",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(activeModule==MOD_HZR||activeModule==MOD_KI||activeModule==MOD_EWM){web.send(501,"text/plain","Fault memory reading not yet verified for this module");return;}if(activeModule==MOD_ABS){readDTCs();web.send(501,"text/plain",dtcText);return;}if(!diagnosticSession||busy){web.send(409,"text/plain","Not connected or busy");return;}activity("Reading fault codes from "+moduleName(activeModule));bool ok=readDTCs();lastDtcScanMs=millis();activity(ok?String("Fault-code read completed"):String("Fault-code read failed: ")+lastError);web.send(ok?200:500,"text/plain",dtcText);});
web.on("/api/test",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(activeModule!=MOD_ENGINE||!diagnosticSession||busy){web.send(409,"text/plain","Connect Engine / CR2 first");return;}String n=web.arg("name"),hex="";if(n=="epc_on")hex="30 55 07 25 1C";else if(n=="epc_off")hex="30 55 07 01 F4";else if(n=="glow_lamp_on")hex="30 56 07 25 1C";else if(n=="glow_lamp_off")hex="30 56 07 01 F4";else if(n=="heater_on")hex="30 1C 07 25 1C";else if(n=="heater_off")hex="30 1C 07 01 F4";else if(n=="fan_on")hex="30 1B 07 03 E8";else if(n=="fan_off")hex="30 1B 07 23 28";else if(n=="ac_cut_on")hex="30 14 07 25 1C";else if(n=="ac_cut_off")hex="30 14 07 01 F4";else if(n=="ac_can_on")hex="30 16 07 25 1C";else if(n=="ac_can_off")hex="30 16 07 01 F4";else if(n=="compression_lrr_on")hex="31 25 00";else if(n=="compression_lrr_off")hex="31 25 01";else if(n=="compression_stop")hex="32 25";else{web.send(400,"text/plain","Unknown test");return;}String label=cr2TestLabel(n);activity("CR2 active test requested: "+label+" ["+hex+"]");String out;bool ok=cr2TestSendHex(hex,out);activity(String(ok?"CR2 active test accepted: ":"CR2 active test failed: ")+label+" — "+out.substring(0,min((int)out.length(),120)));web.send(ok?200:500,"text/plain",out);});
web.on("/api/test/actuator",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(activeModule!=MOD_ENGINE||!diagnosticSession||busy){web.send(409,"text/plain","Connect Engine / CR2 first");return;}int id=strtol(web.arg("id").c_str(),nullptr,10);if(id!=0x11&&id!=0x12&&id!=0x1A&&id!=0x1E&&id!=0x1B){web.send(400,"text/plain","Unsupported actuator");return;}double v=web.arg("value").toDouble();activity("CR2 actuator override requested: local ID 0x"+hex2((uint8_t)id)+" = "+String(v,1)+"%");String out;bool ok=cr2TestActuator((uint8_t)id,v,out);web.send(ok?200:500,"text/plain",out);});
web.on("/api/test/release",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(activeModule!=MOD_ENGINE||!diagnosticSession||busy){web.send(409,"text/plain","Connect Engine / CR2 first");return;}int id=strtol(web.arg("id").c_str(),nullptr,10);if(id!=0x11&&id!=0x12&&id!=0x1A&&id!=0x1E&&id!=0x1B){web.send(400,"text/plain","Unsupported actuator");return;}activity("CR2 actuator release requested: local ID 0x"+hex2((uint8_t)id));String out;bool ok=cr2TestRelease((uint8_t)id,out);web.send(ok?200:500,"text/plain",out);});

web.on("/status",HTTP_GET,[]{web.send(200,"application/json",statusJson());});web.on("/live",HTTP_GET,[]{web.send(200,"application/json",liveJson());});web.on("/dtc",HTTP_GET,[]{web.send(200,"text/plain",dtcText);});web.on("/log",HTTP_GET,[]{web.send(200,"text/plain",logText);});web.on("/api/raw-log.txt",HTTP_GET,[]{web.sendHeader("Content-Disposition","attachment; filename=t1n_scanner_raw_log.txt");web.send(200,"text/plain; charset=utf-8",logText);});web.on("/log/clear",HTTP_POST,[]{logText="";web.send(200,"text/plain","CLEARED");});web.on("/config",HTTP_GET,[]{web.send(200,"application/json",configJson());});
web.on("/config/save",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect){web.send(409,"text/plain","Busy");return;}allRelaysOff();ScannerConfig old=cfg;applyConfigFromWeb();String err=validateConfig();if(err.length()){cfg=old;applyPollGroups();initRelayRouting();web.send(400,"text/plain","NOT SAVED: "+err);return;}autoReconnect=false;livePolling=false;diagnosticSession=false;cr2Connected=false;releaseKL();applyPollGroups();saveScannerConfig();initRelayRouting();lastError="";activity("Transport settings saved; TLIN channel mapping is fixed");setStatus("CONFIG SAVED - DISCONNECTED");web.send(200,"text/plain","Saved");});
web.on("/config/importlab",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect){web.send(409,"text/plain","Busy");return;}importPriorLabTransport();applyPollGroups();saveScannerConfig();autoReconnect=false;livePolling=false;diagnosticSession=false;cr2Connected=false;releaseKL();setStatus("LAB TRANSPORT IMPORTED - DISCONNECTED");web.send(200,"text/plain","Imported proven lab timing");});
web.on("/config/defaults",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect){web.send(409,"text/plain","Busy");return;}allRelaysOff();cfg=ScannerConfig();importPriorLabTransport();applyPollGroups();saveScannerConfig();initRelayRouting();autoReconnect=false;livePolling=false;diagnosticSession=false;cr2Connected=false;releaseKL();lastError="";setStatus("PROVEN DEFAULTS RESTORED - DISCONNECTED");web.send(200,"text/plain","Restored scanner defaults");});
web.on("/connect",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect){web.send(409,"text/plain","Busy");return;}autoReconnect=true;requestedModule=MOD_ENGINE;activeModule=MOD_ENGINE;activeEcuAddr=0x12;jobConnect=true;web.send(202,"text/plain","ENGINE CONNECT QUEUED");});
web.on("/disconnect",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy){web.send(409,"text/plain","Busy");return;}autoReconnect=false;reconnectAtMs=0;disconnectCurrentModule();web.send(200,"text/plain","DISCONNECTED");});
web.on("/poll/start",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(!diagnosticSession){web.send(409,"text/plain","Connect first");return;}livePolling=true;nextPollMs=millis();web.send(200,"text/plain","LIVE POLLING STARTED");});
web.on("/poll/stop",HTTP_POST,[]{if(rejectAutomaticOwner())return;livePolling=false;web.send(200,"text/plain","LIVE POLLING STOPPED");});
web.on("/dtc/read",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(!diagnosticSession||busy){web.send(409,"text/plain","Not connected or busy");return;}bool ok=readDTCs();web.send(ok?200:500,"text/plain",dtcText);});
web.on("/dtc/clear",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(!diagnosticSession||busy){web.send(409,"text/plain","Not connected or busy");return;}bool ok=clearPowertrainDTCs();web.send(ok?200:500,"text/plain",ok?"DTC CLEAR ACCEPTED":lastError);});
web.on("/payload",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(!diagnosticSession||busy){web.send(409,"text/plain","Not connected or busy");return;}String ans=sendManualPayload(web.arg("x"));web.send(ans.startsWith("Failed")?500:200,"text/plain",ans);});
web.on("/api/update-latest",HTTP_POST,[]{if(rejectAutomaticOwner())return;if(busy||jobConnect||diagnosticSession){web.send(409,"text/plain","Pause monitoring and disconnect before firmware maintenance");return;}activity("Firmware update check/install requested");autoReconnect=false;livePolling=false;diagnosticSession=false;releaseKL();String v,u,sha,r;if(!fetchLatestManifest(v,u,sha,r)){web.send(502,"text/plain","Manifest failed: "+r);return;}bool ok=installFirmwareFromUrl(u,sha,r);web.sendHeader("Connection","close");web.send(ok?200:500,"text/plain",ok?"Installed "+v+". "+r+". Rebooting...":r);if(ok){delay(800);ESP.restart();}});
web.on("/api/update",HTTP_POST,[]{
  if(!firmwareUploadAllowed){web.send(409,"text/plain","Pause automatic monitoring and disconnect before uploading a .bin");return;}
  bool ok=firmwareUploadComplete&&!Update.hasError();web.sendHeader("Connection","close");
  web.send(ok?200:500,"text/plain",ok?"Firmware installed. Rebooting...":String("Update failed: ")+Update.errorString());
  firmwareUploadAllowed=false;if(ok){delay(700);ESP.restart();}
},[]{
  HTTPUpload&up=web.upload();
  if(up.status==UPLOAD_FILE_START){
    firmwareUploadAllowed=false;firmwareUploadComplete=false;
    if(monEnabled||busy||jobConnect||diagnosticSession||!up.filename.endsWith(".bin"))return;
    firmwareUploadAllowed=Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH);
  }else if(up.status==UPLOAD_FILE_WRITE){
    if(firmwareUploadAllowed&&!Update.hasError())Update.write(up.buf,up.currentSize);
  }else if(up.status==UPLOAD_FILE_END){
    if(firmwareUploadAllowed&&!Update.hasError())firmwareUploadComplete=Update.end(true);
  }else if(up.status==UPLOAD_FILE_ABORTED&&firmwareUploadAllowed){Update.abort();firmwareUploadAllowed=false;}
});
}
void setup(){Serial.begin(115200);pinMode(RX_PIN,INPUT);loadAllSettings();applyWifiSettings();initRelayRouting();releaseKL();// Connect the station *before* starting SoftAP. A single-radio ESP32 otherwise
// has to arbitrate channels while authenticating to AntNet.
WiFi.mode(WIFI_STA);WiFi.setSleep(false);WiFi.onEvent(wifiEventLog);
WiFi.setAutoReconnect(true);
lastWifiRetryMs=millis();
if(wifiSSID.length()){
 wifiConnectAttempts++;WiFi.begin(wifiSSID.c_str(),wifiPASS.c_str());
 uint32_t t=millis();while(WiFi.status()!=WL_CONNECTED&&millis()-t<12000)delay(100);
}
// Always bring up the recovery AP, regardless of station success.
WiFi.mode(WIFI_AP_STA);WiFi.softAP(apSSID.c_str(),apPASS.c_str());
sdInitialize();historyBegin();monitorBegin();alertsBegin();setupRoutes();web.begin();activity("T1N Scanner "+String(FIRMWARE_VERSION)+" booted");activity("Boot reset reason code "+String((int)esp_reset_reason()));logx(String("T1N Scanner ")+FIRMWARE_VERSION+" ready");logx(String("AP http://")+WiFi.softAPIP().toString()+" | station "+(WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():String("not connected")));setStatus("DISCONNECTED - SELECT MODULE");}
void loop(){web.handleClient();hzrFanTick();if(jobConnect&&!busy){jobConnect=false;connectModule(requestedModule);}livePollTick();historyQueryTick();historyTick();monitorTick();alertsTick();if(diagnosticSession&&!busy&&(activeModule==MOD_ENGINE||activeModule==MOD_EGS)&&millis()-lastDtcScanMs>=60000){readDTCs();lastDtcScanMs=millis();}if(diagnosticSession&&activeModule!=MOD_ABS&&!busy&&!livePolling&&millis()-lastGoodTrafficMs>=cfg.keepaliveIntervalMs){busy=true;testerPresent();busy=false;}if(!diagnosticSession&&autoReconnect&&reconnectAtMs&&(int32_t)(millis()-reconnectAtMs)>=0&&!busy&&!jobConnect){reconnectAtMs=0;jobConnect=true;setStatus("AUTO RECONNECT QUEUED - "+moduleName(requestedModule));}if(wifiSSID.length()&&WiFi.status()!=WL_CONNECTED&&millis()-lastWifiRetryMs>90000){
 lastWifiRetryMs=millis();wifiConnectAttempts++;
 // Do not forcibly disconnect an already-connecting station every 30 seconds.
 WiFi.begin(wifiSSID.c_str(),wifiPASS.c_str());
}delay(2);}
