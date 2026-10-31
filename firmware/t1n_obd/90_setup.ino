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

#include "shared_client_generated.h"
void setupRoutes(){
 // Automatic monitoring owns the UART, including between connection attempts.
 const char* manualRoutes[]={"/api/monitor/enable","/api/module/read","/api/relay/test","/api/shared-probe/run","/api/shared-probe/timing","/api/module/connect","/api/hzr/fan/status","/api/hzr/fan/on","/api/disconnect","/api/poll/start","/api/poll/stop","/api/dtc/read","/api/test","/api/test/actuator","/api/test/release","/config/save","/config/importlab","/config/defaults","/connect","/disconnect","/poll/start","/poll/stop","/dtc/read","/dtc/clear","/payload"};
 for(const char* path:manualRoutes)web.on(path,HTTP_POST,[]{
   web.send(409,"application/json","{\"ok\":false,\"code\":\"AUTOMATIC_MONITORING\",\"error\":\"Automatic monitoring owns diagnostics; manual commands are unavailable\"}");
 });

web.on("/t1n-data.js",HTTP_GET,[]{
  web.sendHeader("Cache-Control","no-store");
  web.send_P(200,"application/javascript; charset=utf-8",T1N_SHARED_CLIENT);
});
web.on("/pi",HTTP_GET,[]{
  if(sdServePage("/www/pi.html"))return;
  web.send(503,"text/html; charset=utf-8","<!doctype html><meta name=viewport content=width=device-width><title>T1N display unavailable</title><h2>Pi display unavailable</h2><p>Mount the microSD card and install the matching dashboard files.</p><p><a href='/'>Open scanner</a></p><script>setTimeout(()=>location.reload(),10000)</script>");
});
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

web.on("/api/hardware/lines",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",klineLogicJson());});
web.on("/api/modules",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",modulesCatalogJson());});
web.on("/api/module/groups",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",moduleGroupsJson());});

web.on("/wifi",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"text/html; charset=utf-8",wifiPage());});
web.on("/api/wifi",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",wifiDiagnosticsJson());});
web.on("/api/wifi/scan",HTTP_GET,[]{
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
web.on("/api/sensors",HTTP_GET,[]{sensorsSend();});
web.on("/api/history/status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",historyStatusJson());});
web.on("/api/history/data",HTTP_GET,[]{
  web.sendHeader("Cache-Control","no-store");
  String module=web.arg("module");
  if(module.length()!=1||module[0]<'0'||module[0]>'5'){
    web.send(400,"application/json","{\"ok\":false,\"code\":\"INVALID_MODULE\",\"error\":\"Module must be 0 through 5\",\"points\":[]}");return;
  }
  historyDataSend((uint8_t)(module[0]-'0'),web.arg("id"),web.arg("range"));
});
web.on("/api/history/clock",HTTP_POST,[]{
  String epoch=web.arg("epoch");
  if(epoch.length()<10||epoch.length()>10){web.send(400,"text/plain","Invalid UTC time");return;}
  char *end=nullptr;unsigned long long value=strtoull(epoch.c_str(),&end,10);
  if(!end||*end||value>UINT32_MAX||!historySetClock((uint32_t)value)){web.send(400,"text/plain","Invalid UTC time");return;}
  web.send(200,"text/plain","Clock synchronized");
});

web.on("/sensors",HTTP_GET,[]{sensorsSend();});
web.on("/api/dtc",HTTP_GET,[]{web.send(200,"text/plain",dtcText);});
web.on("/api/activity-log",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"text/plain; charset=utf-8",activityLogText);});
web.on("/api/activity-log.txt",HTTP_GET,[]{web.sendHeader("Content-Disposition","attachment; filename=t1n_scanner_activity_log.txt");web.send(200,"text/plain; charset=utf-8",activityLogText);});
web.on("/api/activity-log/clear",HTTP_POST,[]{activityLogText="";activity("Activity log cleared");web.send(200,"text/plain","Cleared");});
web.on("/api/relay/status",HTTP_GET,[]{web.sendHeader("Cache-Control","no-store");web.send(200,"application/json",relayDiagnosticsJson());});

web.on("/api/shared-probe/targets",HTTP_GET,[]{web.send(200,"application/json",sharedProbeListJson());});

#ifdef T1N_ENABLE_PIN15_TIMING_LAB

#endif

 web.on("/api/hzr/fan/state",HTTP_GET,[]{web.send(200,"application/json",hzrFanFeedbackJson());});

web.on("/status",HTTP_GET,[]{web.send(200,"application/json",statusJson());});web.on("/live",HTTP_GET,[]{web.send(200,"application/json",liveJson());});web.on("/dtc",HTTP_GET,[]{web.send(200,"text/plain",dtcText);});web.on("/log",HTTP_GET,[]{web.send(200,"text/plain",logText);});web.on("/api/raw-log.txt",HTTP_GET,[]{web.sendHeader("Content-Disposition","attachment; filename=t1n_scanner_raw_log.txt");web.send(200,"text/plain; charset=utf-8",logText);});web.on("/log/clear",HTTP_POST,[]{logText="";web.send(200,"text/plain","CLEARED");});web.on("/config",HTTP_GET,[]{web.send(200,"application/json",configJson());});

web.on("/api/update-latest",HTTP_POST,[]{if(busy){web.send(409,"text/plain","Scanner busy");return;}activity("Firmware update check/install requested");livePolling=false;diagnosticSession=false;releaseKL();String v,u,sha,r;if(!fetchLatestManifest(v,u,sha,r)){web.send(502,"text/plain","Manifest failed: "+r);return;}bool ok=installFirmwareFromUrl(u,sha,r);web.sendHeader("Connection","close");web.send(ok?200:500,"text/plain",ok?"Installed "+v+". "+r+". Rebooting...":r);if(ok){delay(800);ESP.restart();}});
web.on("/api/update",HTTP_POST,[]{bool ok=!Update.hasError();web.sendHeader("Connection","close");web.send(ok?200:500,"text/plain",ok?"Firmware installed. Rebooting...":String("Update failed: ")+Update.errorString());if(ok){delay(700);ESP.restart();}},[]{HTTPUpload&up=web.upload();if(up.status==UPLOAD_FILE_START){livePolling=false;diagnosticSession=false;releaseKL();if(!up.filename.endsWith(".bin")){Update.abort();return;}Update.begin(UPDATE_SIZE_UNKNOWN,U_FLASH);}else if(up.status==UPLOAD_FILE_WRITE){if(!Update.hasError())Update.write(up.buf,up.currentSize);}else if(up.status==UPLOAD_FILE_END){if(!Update.hasError())Update.end(true);}else if(up.status==UPLOAD_FILE_ABORTED)Update.abort();});
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
sdInitialize();historyBegin();monitorBegin();setupRoutes();web.begin();activity("T1N Scanner "+String(FIRMWARE_VERSION)+" booted");logx(String("T1N Scanner ")+FIRMWARE_VERSION+" ready");logx(String("AP http://")+WiFi.softAPIP().toString()+" | station "+(WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():String("not connected")));setStatus("DISCONNECTED - SELECT MODULE");}
void loop(){web.handleClient();hzrFanTick();livePollTick();historyTick();historyQueryTick();monitorTick();if(diagnosticSession&&!busy&&(activeModule==MOD_ENGINE||activeModule==MOD_EGS)&&millis()-lastDtcScanMs>=60000){readDTCs();lastDtcScanMs=millis();}if(diagnosticSession&&activeModule!=MOD_ABS&&!busy&&!livePolling&&millis()-lastGoodTrafficMs>=cfg.keepaliveIntervalMs){busy=true;testerPresent();busy=false;}if(wifiSSID.length()&&WiFi.status()!=WL_CONNECTED&&millis()-lastWifiRetryMs>90000){
 lastWifiRetryMs=millis();wifiConnectAttempts++;
 // Do not forcibly disconnect an already-connecting station every 30 seconds.
 WiFi.begin(wifiSSID.c_str(),wifiPASS.c_str());
}delay(2);}
