#ifndef T1N_USE_GZIP_PAGE
String scannerPage(){return R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><meta name="theme-color" content="#08111f"><title>T1N Scanner</title><style>
:root{--bg:#07101c;--panel:#0d1928;--line:#24374f;--text:#eef6ff;--muted:#8ea2ba;--blue:#2b7fff;--green:#2fc477;--red:#ef5b62}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:15px system-ui,-apple-system,Segoe UI,sans-serif}.wrap{max-width:1120px;margin:auto;padding:14px 14px 90px}.top{display:flex;justify-content:space-between;gap:12px;align-items:center;padding:5px 0 14px}.brand{font-size:22px;font-weight:850}.sub{color:var(--muted);font-size:12px}.statusDot{width:10px;height:10px;border-radius:50%;display:inline-block;background:#64748b;margin-right:7px}.statusDot.ok{background:var(--green);box-shadow:0 0 12px #2fc47788}.statusDot.bad{background:var(--red)}.statusDot.wait{background:#e7a93b;box-shadow:0 0 12px #e7a93b66}.nav{display:grid;grid-template-columns:repeat(6,1fr);gap:7px;background:#091523;border:1px solid var(--line);padding:6px;border-radius:14px;position:sticky;top:6px;z-index:10}.nav button{background:transparent;color:var(--muted);border:0;border-radius:10px;padding:10px 4px;font-weight:750}.nav button.on{background:#173252;color:#fff}.view{display:none}.view.on{display:block}.hero{margin:12px 0;background:linear-gradient(145deg,#10233a,#0c1725);border:1px solid var(--line);border-radius:16px;padding:16px}.heroRow{display:flex;justify-content:space-between;gap:12px;align-items:flex-start}.moduleName{font-size:23px;font-weight:850}.badge{display:inline-flex;align-items:center;border:1px solid var(--line);border-radius:999px;padding:5px 9px;color:#b9c8d8;font-size:12px}.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:14px}button{border:0;border-radius:10px;padding:10px 13px;background:#24415f;color:#fff;font-weight:800;cursor:pointer}.primary{background:var(--blue)!important}.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(165px,1fr));gap:9px;margin:10px 0}.metric{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:13px;min-height:116px}.metric{cursor:pointer;transition:border-color .15s,transform .15s}.metric:active{transform:scale(.99)}.metric.stale{opacity:.48}.metric.ok{border-color:#2f8f63}.metric.warn{border-color:#b88a36}.metric.bad{border-color:#c34f58}.metric .label{color:#9eb0c5;font-size:12px;min-height:30px}.metric .value{font-size:27px;font-weight:850;line-height:1.15;margin-top:4px}.metricNumber,.metric .unit{display:inline-block;white-space:nowrap}.pairedCard{grid-column:span 2;min-width:0}.metric .unit{font-size:12px;color:#8296ad;margin-left:3px}.metric .imperial{font-size:12px;color:#a8bad0;margin-top:3px}.metric .expected{font-size:11px;color:#8296ad;line-height:1.35;margin-top:8px}.pairMetric{min-height:150px}.pairGrid{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:10px;margin-top:6px}.pairSide{background:#091421;border:1px solid #1b2d42;border-radius:10px;padding:9px}.pairTag{font-size:10px;letter-spacing:.08em;color:#71869e;font-weight:850}.pairValue{font-size:22px;font-weight:850;line-height:1.15;margin-top:3px}.compare{font-size:12px;font-weight:750;margin-top:9px;color:#a8bad0}.metric.ok .compare{color:#8fd6b3}.metric.warn .compare{color:#e4bf73}.metric.bad .compare{color:#f29aa0}.metric.ok .expected{color:#8fd6b3}.metric.warn .expected{color:#e4bf73}.metric.bad .expected{color:#f29aa0}.sectionTitle{font-size:17px;font-weight:850;margin:18px 2px 8px}.moduleGrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:10px}.module{background:var(--panel);border:1px solid var(--line);border-radius:15px;padding:14px}.module.active{border-color:#3b82f6;box-shadow:inset 0 0 0 1px #3b82f633}.module h3{margin:0 0 5px}.pin{font-size:24px;font-weight:850;color:#8dc0ff}.note{color:var(--muted);font-size:12px;line-height:1.45}.faultBox{white-space:pre-wrap;background:#050b12;border:1px solid var(--line);border-radius:13px;padding:13px;min-height:130px;font:13px ui-monospace,monospace;color:#dce9f7}.notice{border:1px solid #6b5229;background:#251d11;color:#ffd999;border-radius:12px;padding:10px 12px;margin:10px 0}.error{color:#ff9ca2;font-size:13px;margin-top:8px}.footerLink{display:block;text-align:center;margin-top:22px;color:#91bfff;text-decoration:none}.tiny{font-size:11px;color:#71869e}.spinner{display:inline-block;width:14px;height:14px;border:2px solid #ffffff55;border-top-color:#fff;border-radius:50%;animation:spin .8s linear infinite;vertical-align:-2px;margin-right:6px}@keyframes spin{to{transform:rotate(360deg)}}.logBtns{display:flex;gap:8px;flex-wrap:wrap;margin:10px 0}@media(max-width:560px){.wrap{padding:10px 10px 80px}.nav{grid-template-columns:repeat(3,1fr)}.nav button{font-size:12px}.moduleName{font-size:20px}.metric .value{font-size:24px}}@media(max-width:380px){.pairedCard{grid-column:1/-1}}
 .trendBox{background:#0c1929;border:1px solid #315170;border-radius:15px;padding:15px;margin:12px 0}.trendBox svg{width:100%;height:auto;min-height:160px;display:block}.trendTitle{font-size:17px;font-weight:800}.trendBox{scroll-margin-top:105px}.historyLoading{min-height:160px;display:grid;align-content:center;gap:8px;padding:16px;border:1px dashed var(--line);border-radius:10px;margin-top:10px}.testResultGood{border-color:#248458;background:#0b231d}.testResultBad{border-color:#ac5158;background:#281518}.testResultWait{border-color:#997d38;background:#241f13}
.multiPicker{display:none;border:1px solid var(--line);background:#071321;border-radius:12px;padding:8px;margin-top:10px}.multiPicker.open{display:block}.multiPickerHeader{display:flex;align-items:center;justify-content:space-between;gap:8px;margin-bottom:8px}.multiPickerHeader button{min-height:44px}.multiOptions{max-height:235px;overflow:auto;overscroll-behavior:contain;display:grid;grid-template-columns:repeat(auto-fit,minmax(205px,1fr));gap:5px}.multiPicker input[type="search"]{width:100%;background:#081626;color:#fff;border:1px solid #36506d;padding:10px;border-radius:8px;margin-bottom:8px;font-size:16px}.multiOption{padding:7px;display:flex;align-items:center;gap:8px;font-size:12px;border-radius:7px}.multiOption:has(input:checked){background:#173252}.multiOption input{accent-color:#51a7ff}.multiLegend{display:flex;flex-wrap:wrap;gap:9px;margin:9px 0}.multiLegend span{font-size:12px;padding:5px 8px;background:#11253a;border-radius:8px}.multiControls{display:flex;gap:8px;flex-wrap:wrap;align-items:center}.multiControls label{font-size:13px;color:#c7d9ec;display:flex;align-items:center;gap:4px}.multiControls input[type="checkbox"]{accent-color:#51a7ff}.trendBox svg.multiGraph{width:100%;height:auto;max-height:320px;min-height:190px;display:block;margin-top:10px}.multiHint{font-size:12px;color:#9aaec4;margin-top:6px}
</style></head><body><div class="wrap"><div class="top"><div><div class="brand">T1N Scanner</div><div class="sub">2003 Sprinter diagnostic scanner · <span id="fw">...</span></div></div><div class="badge"><span id="dot" class="statusDot"></span><span id="linkState">Disconnected</span></div></div>
<div class="nav"><button class="on" data-v="overview">Overview</button><button data-v="live">Live Data</button><button data-v="faults">Fault Codes</button><button data-v="tests">Tests</button><button data-v="modules">Modules</button><button data-v="logs">Logs</button></div>
<section id="overview" class="view on"><div class="hero"><div class="heroRow"><div><div class="sub">ACTIVE MODULE</div><div id="moduleName" class="moduleName">Engine / CR2</div><div class="sub">Address <span id="addr">0x12</span> · OBD pin <b id="pin">7</b> · TLIN channel <b id="relay">K2</b> / RX GPIO <b id="relayGpio">37</b></div><div id="variantLine" class="sub" style="display:none">ECU variant <b id="variantId">--</b></div></div><span id="pollBadge" class="badge">Live data off</span></div><div id="err" class="error"></div><div id="sensorError" class="error"></div><div id="recordingHealth" class="note">Checking microSD recording…</div><div class="actions"><button id="connectBtn" class="primary" onclick="connectActive()">Connect</button><button id="disconnectBtn" onclick="disconnectActive()">Disconnect</button><button onclick="showView('modules')">Choose Module</button></div></div><div class="sectionTitle">Key data</div><div id="keyCards" class="cards"></div><div class="note">Metric and imperial values are shown together where a useful conversion exists. Actual/target pairs share one card. Differences require fresh, time-aligned measurements; missing data never means healthy.</div></section>
<section id="live" class="view"><div class="sectionTitle">Live Data · <span id="liveModule">...</span></div><div class="actions"><button class="primary" onclick="pollOn()">Start live data</button><button onclick="pollOff()">Stop</button></div><div class="note">Tap a measurement for saved microSD history, then add up to eight series. Live gauges use current ESP32 readings; charts and exports use saved records only.</div><div id="trend" class="trendBox" style="display:none"><div class="heroRow"><div><div id="trendTitle" class="trendTitle">History</div><div id="trendSubtitle" class="sub"></div></div><button onclick="closeTrend()">Close</button></div><div class="actions"><button id="multiPickerToggle" class="primary" aria-controls="multiPicker" aria-expanded="false" onclick="toggleMultiPicker()">+ Add metrics</button><button onclick="exportMultiCSV()">Export CSV</button><button onclick="exportMultiSVG()">Export SVG</button><button onclick="clearMultiMetrics()">Reset chart</button></div>
<div class="multiControls" style="margin-top:10px"><label><input id="multiPairTargets" type="checkbox" checked onchange="multiAutoTargets=this.checked;multiSelectionChanged()">Include target automatically</label></div>
<div id="multiPicker" class="multiPicker" aria-label="Add or remove measurements"><div class="multiPickerHeader"><span id="multiSelectionCount" class="note"></span><button id="multiPickerDone" class="primary" onclick="setMultiPicker(false,true)">Done</button></div><input id="multiSearch" type="search" placeholder="Find a measurement..." aria-label="Search chart measurements" oninput="renderMultiPicker()" onkeydown="if(event.key==='Escape')setMultiPicker(false,true)"><div id="multiOptions" class="multiOptions"></div></div>
<div class="actions" id="historyRanges">
<button onclick="setTrendRange('1h')">1 hour</button>
<button onclick="setTrendRange('24h')">24 hours</button>
<button onclick="setTrendRange('7d')">7 days</button>
<button onclick="setTrendRange('30d')">30 days</button>
<button onclick="setTrendRange('all')">All</button>
</div><div id="trendPlot" aria-live="polite" aria-label="Saved microSD history"></div><div id="trendLegend" class="note"></div><div class="note" id="trendFoot"></div><div class="note" id="historyStorage">Checking microSD storage…</div><button onclick="syncHistoryClock()">Set clock from this device</button></div><div id="allCards" class="cards"></div><div class="sectionTitle">ECU diagnostic groups / raw responses</div><div class="note">Raw frames are preserved for every captured group. Unlabelled bytes are displayed without guessing units or physical meaning. Stop live polling before reading one group manually.</div><div id="liveGroupCatalog" class="moduleGrid"></div></section>
<section id="faults" class="view"><div class="sectionTitle">Fault Codes · <span id="faultModule">...</span></div><div class="actions"><button id="absReadBtn" class="primary" onclick="readDtc()">Read fault codes</button></div><div id="absDtcNote" class="notice" style="display:none">ABS fault-code definitions are available below. The ECU rejected the standard 18 02 FF 00 read request (NRC 12). An Autel fault-screen capture is needed before enabling stored-fault reads. No fault codes have been read or cleared.</div><div id="absFaultLookup" class="module" style="display:none"><b>ABS code lookup (CBF catalog)</b><div class="note">Search a code or description; this is a reference, not a live fault list.</div><input id="absCode" placeholder="C1401 or wheel sensor" oninput="renderAbsFaultLookup()" style="width:100%;max-width:360px;padding:9px;background:var(--panel);border:1px solid var(--line);border-radius:8px;color:inherit;margin:8px 0"><div id="absDtcResult" style="max-height:270px;overflow:auto"></div></div><div id="faultText" class="faultBox">No fault-code read yet.</div><div class="note" style="margin-top:8px">Fault clearing and raw commands stay in Advanced so normal scanner use remains read-only.</div></section>
<section id="tests" class="view"><div id="hzrFanPanel" style="display:none"><div class="sectionTitle">A/C Auxiliary Fan · HZRKL</div><div class="module"><div class="note">OBD pin 15 · module 0x50 · fan control is experimental. Use ignition ON, vehicle parked, and keep hands away from the fan.</div><div class="actions"><button class="primary" onclick="hzrStatus()">Read Fan Status</button><button onclick="hzrStart()">Run Fan (4 seconds)</button></div><pre id="hzrFanStatus" class="faultBox" style="margin-top:10px;min-height:50px">Read fan status to check command output.</pre><div id="hzrFanOutcome" class="note"></div></div></div><div id="cr2TestPanel"><div class="sectionTitle">CR2 Active Tests</div><div class="sectionTitle" style="font-size:14px">Live ECU feedback</div><div id="testFeedback" class="cards"></div><div class="note">These values are read back from the ECU while live polling is active. A positive test response means the ECU accepted the command; it does not by itself prove a lamp, fan, valve, or actuator physically moved.</div><div id="testOnlyEngine" class="notice" style="display:none">These controls require Engine / CR2 on OBD pin 7.</div><div class="moduleGrid"><div class="module"><h3>Output tests</h3><div class="actions"><button onclick="testCmd('epc_on')">EPC lamp ON</button><button onclick="testCmd('epc_off')">EPC lamp OFF</button><button onclick="testCmd('glow_lamp_on')">Glow lamp ON</button><button onclick="testCmd('glow_lamp_off')">Glow lamp OFF</button><button onclick="testCmd('heater_on')">Aux heater ON</button><button onclick="testCmd('heater_off')">Aux heater OFF</button><button onclick="testCmd('fan_on')">Fan ON</button><button onclick="testCmd('fan_off')">Fan OFF</button><button onclick="testCmd('ac_cut_on')">A/C cutoff ON</button><button onclick="testCmd('ac_cut_off')">A/C cutoff OFF</button><button onclick="testCmd('ac_can_on')">A/C CAN ON</button><button onclick="testCmd('ac_can_off')">A/C CAN OFF</button></div></div><div class="module"><h3>Actuator override</h3><div class="note">Set a percentage, send it, then release control back to the ECU.</div><div id="actuatorControls"></div></div><div class="module"><h3>Compression test</h3><div class="actions"><button class="primary" onclick="testCmd('compression_lrr_on')">Start · LRR active</button><button class="primary" onclick="testCmd('compression_lrr_off')">Start · LRR disabled</button><button onclick="testCmd('compression_stop')">Stop</button></div></div></div><div class="sectionTitle">Last test result</div><div id="testResult" class="faultBox">No test sent yet.</div><div id="testExplain" class="note" style="margin-top:8px">The ECU response confirms acceptance or rejection. A physical outcome requires an appropriate live sensor changing.</div></div></section><section id="modules" class="view"><div class="sectionTitle">All ECUs on the four wired K-lines</div><div class="note">Multiple diagnostic addresses share a physical OBD pin; select the ECU, not the cable. Entries are based on successful vehicle or Autel communication, not assumed scan addresses.</div><div id="moduleCatalog" class="moduleGrid"></div><div class="notice">Pin 7: engine; pin 9: ABS + shift lever; pin 11: transmission; pin 15: HVAC + instrument cluster. ABS uses KWFB and its detailed live transport remains under investigation. EWM has a confirmed fault/status summary but no validated continuous sensor map.</div></section>
<section id="logs" class="view"><div class="sectionTitle">Diagnostic Activity Log</div><div class="note">Human-readable scanner events. Raw KWP traffic is still available in Advanced.</div><div class="logBtns"><button id="pauseLogBtn" onclick="toggleLog()">Pause</button><button onclick="copyLog()">Copy</button><button onclick="exportLog()">Export .txt</button><button onclick="clearActivityLog()">Clear</button></div><pre id="activityLog" class="faultBox" style="min-height:360px;max-height:65vh;overflow:auto">Loading...</pre></section><a class="footerLink" href="/advanced">Advanced tools, raw terminal, settings & firmware update →</a><div class="tiny" style="text-align:center;margin-top:8px">Active tests are only sent when you press a control.</div></div>
<script src="/t1n-data.js"></script>
<script>const absDtcCatalog=`C1000|ABS controller internal
C1010|Low battery voltage
C1011|Valve relay or supply
C1020|CAN initialization/communication
C1022|CAN participants 210/212/218/240/308/310/312/608
C1024|CAN participant 408
C1100|Front-left wheel sensor open circuit
C1101|Front-right wheel sensor open circuit
C1102|Rear-left wheel sensor open circuit
C1103|Rear-right wheel sensor open circuit
C1104|Front-left wheel sensor implausible
C1105|Front-right wheel sensor implausible
C1106|Rear-left wheel sensor implausible
C1107|Rear-right wheel sensor implausible
C1200|Brake-light switch
C1300|Front-left inlet valve
C1301|Front-left outlet valve
C1302|Front-right inlet valve
C1303|Front-right outlet valve
C1304|Rear-left inlet valve
C1305|Rear-left outlet valve
C1306|Rear-right inlet valve
C1307|Rear-right outlet valve
C1311|Switchover valve 1
C1312|Switchover valve 2
C1313|Shutoff valve 1
C1314|Shutoff valve 2
C1401|ABS pump motor relay or motor supply
C1407|Shutoff EEPROM checksum
C1408|Pressure sensor
C1409|Options coding
C140A|ASG present, AAS missing
C1410|ASG present, AAS missing
C1411|Retarder shutoff relay
C1500|Incorrect tone wheel or tire size
P0000|Retarder relay (CBF alias, unverified)`.trim().split('\n').map(x=>x.split('|'));
function renderAbsFaultLookup(){const q=$('absCode').value.trim().toUpperCase(),matches=absDtcCatalog.filter(x=>x.join(' ').toUpperCase().includes(q));$('absDtcResult').innerHTML=matches.length?matches.map(x=>'<div style="padding:5px 0;border-bottom:1px solid var(--line)"><b>'+esc(x[0])+'</b> · '+esc(x[1])+'</div>').join(''):'No matching ABS code';}
const client=T1NData.createClient();
let S={moduleId:0,connected:0,polling:0},modulesCatalog=[],lastGroupUpdate=0,lastGroupModule=-1,items=[],connecting=false,logPaused=false,
selectedTrend=null,chartModule=0,multiPicked=[],multiAutoTargets=true,multiStored={},multiFetchedAt=0,multiFetchToken=0,
trendRange='1h',historyStorageChecked=0,snapshotReceivedMs=0,scannerOnline=false,sensorError='',refreshRunning=false,refreshTimer=null,
historyLoading=false,historyLoadedCount=0,historyError='',historyLoadPromise=null;
const metadataByModule={};
const $=x=>document.getElementById(x);document.querySelectorAll('.nav button').forEach(b=>b.onclick=()=>showView(b.dataset.v));function showView(v){document.querySelectorAll('.view').forEach(x=>x.classList.toggle('on',x.id===v));document.querySelectorAll('.nav button').forEach(x=>x.classList.toggle('on',x.dataset.v===v));}function esc(s){return String(s??'--').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}function fmt(v,d){return Number(v).toFixed(d)}function imperial(x){if(!x||!x.valid)return'';let v=Number(x.value),u=x.unit||'';if(!Number.isFinite(v))return'';if(u==='°C')return fmt(v*9/5+32,1)+' °F';if(u==='km/h')return fmt(v*0.621371,1)+' mph';if(u==='km')return fmt(v*0.621371,0)+' mi';if(u==='bar')return fmt(v*14.5037738,x.id&&x.id.includes('lowFuel')?1:0)+' psi';if(u==='hPa'){if(x.id==='atmos')return fmt(v*0.0295299831,2)+' inHg';return fmt(v*0.0145037738,1)+' psi'}if(u==='mbar')return fmt(v*0.0145037738,1)+' psi';if(u==='Nm')return fmt(v*0.737562149,0)+' lb-ft';if(u==='mm')return fmt(v*0.0393700787,2)+' in';if(u==='kg/h')return fmt(v*2.20462262,1)+' lb/hr';if(u==='g/s')return fmt(v*7.93664144,1)+' lb/hr';if(u==='mg/stroke')return fmt(v*0.0154323584,3)+' gr/stroke';if(u==='mm³/stroke')return fmt(v*0.0000610237441,6)+' in³/stroke';return''}function primaryValue(x,cls='value'){let imp=imperial(x);return `<div class="${cls}"><span class="metricNumber">${esc(x&&x.valid?x.value:'--')}</span><span class="unit">${esc(x&&x.valid?x.unit:'')}</span></div>${imp?`<div class="imperial">${esc(imp)}</div>`:''}`}function card(x){let fresh=T1NData.sensorFresh(x,snapshotReceivedMs,Date.now()),st=fresh?(x.state||'neutral'):'neutral';return `<div class="metric ${fresh?'':'stale'} ${esc(st)}"><div class="label">${esc(x.label)}${fresh?'':' · '+(x.valid?'STALE':'UNAVAILABLE')}</div>${primaryValue(x)}<div class="expected">${esc(x.expected||'')}</div></div>`}const pairSpecs=T1NData.pairSpecs;let byId={};
const pairedTargets=new Set(Object.values(pairSpecs).map(s=>s.target));
function pairEval(a,t,s){
  if(!T1NData.sensorFresh(a,snapshotReceivedMs,Date.now())||!T1NData.sensorFresh(t,snapshotReceivedMs,Date.now()))
    return {state:'neutral',text:'Comparison unavailable: measurements missing or stale'};
  if(Math.abs(a.sampleMs-t.sampleMs)>2000)return {state:'neutral',text:'Live Δ unavailable · samples '+fmt(Math.abs(a.sampleMs-t.sampleMs)/1000,1)+' s apart. Saved charts remain available.'};
  if(s.mode==='gear')return {state:'neutral',text:a.value===t.value?'Actual matches target':'Actual gear differs from target'};
  const d=T1NData.pairDelta(a,t,{receivedMs:snapshotReceivedMs,nowMs:Date.now()});
  return {state:'neutral',text:d.valid?'Δ '+(d.value>=0?'+':'')+fmt(d.value,Math.abs(d.value)<10?2:0)+' '+d.unit:d.reason};
}
function pairCard(a,t,s){let ev=pairEval(a,t,s),stale=!T1NData.sensorFresh(a,snapshotReceivedMs,Date.now())||!T1NData.sensorFresh(t,snapshotReceivedMs,Date.now());return `<div class="metric pairMetric ${stale?'stale':''} ${ev.state}"><div class="label">${esc(s.title)}${stale?' · STALE / UNAVAILABLE':''}</div><div class="pairGrid"><div class="pairSide"><div class="pairTag">ACTUAL</div>${primaryValue(a,'pairValue')}</div><div class="pairSide"><div class="pairTag">${esc(s.targetLabel||'TARGET')}</div>${primaryValue(t,'pairValue')}</div></div><div class="compare">${esc(ev.text)}</div></div>`}function renderItems(xs){return xs.filter(x=>!pairedTargets.has(x.id)).map(x=>{let s=pairSpecs[x.id],t=s?byId[s.target]:null;return `<div data-metric-id="${esc(x.id)}" class="${s&&t?'pairedCard':''}">${s&&t?pairCard(x,t,s):card(x)}</div>`}).join('')}function keyItems(){let ids=S.moduleId===0?['rpm','speed','coolant','battery','lowFuel','boost','rail']:S.moduleId===2?['temp','gear','converterSlip','shiftCurrentActual','modCurrentActual','speed','battery']:S.moduleId===1?['wheelFL','wheelFR','wheelRL','wheelRR','voltage','brakeLamp']:S.moduleId===5?['kiFuel','kiOutside','kiDimmer','kiSpeed','kiSender']:S.moduleId===3?['g2164_b0','g2164_b1','g2164_b2']:[];return ids.map(id=>byId[id]).filter(Boolean)}function metadataFor(module){
  const all=new Map((T1NData.metricCatalog[module]||[]).map(x=>[x.id,x]));
  for(const x of metadataByModule[module]||[])all.set(x.id,x);
  return [...all.values()];
}
function closeTrend(){selectedTrend=null;multiPicked=[];multiStored={};multiFetchToken++;historyError='';historyLoading=false;$('trend').style.display='none';setMultiPicker(false);}
async function checkHistoryStorage(){
  historyStorageChecked=Date.now();
  try{
    const d=await client.historyStatus();
    const error=d.error||(!d.available?'microSD history unavailable':'');
    const text=error?'microSD history: '+error:'microSD history ready · '+(d.timeReady?'UTC clock valid':'UTC clock unavailable');
    $('historyStorage').textContent=text;
    $('recordingHealth').textContent=text+' · Recording policy: changes at least 10 seconds apart; unchanged-value heartbeat 30 minutes.';
  }catch(e){$('historyStorage').textContent='microSD history error: '+e.message;$('recordingHealth').textContent=$('historyStorage').textContent;}
}
async function syncHistoryClock(){
  $('historyStorage').textContent='Setting clock…';
  try{
    const r=await fetch('/api/history/clock',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({epoch:Math.floor(Date.now()/1000)})});
    if(!r.ok)throw Error(await r.text());
    client.invalidateHistory();await checkHistoryStorage();if(selectedTrend)await loadMultiHistory(true);
  }catch(e){$('historyStorage').textContent='Clock synchronization failed: '+e.message;}
}
function updateTrendButtons(){
  $('historyRanges').querySelectorAll('button').forEach(b=>{
    const m=(b.getAttribute('onclick')||'').match(/'([^']+)'/);
    b.classList.toggle('primary',!!m&&m[1]===trendRange);
  });
}
function setTrendRange(range){
  if(!['1h','24h','7d','30d','all'].includes(range))return;
  trendRange=range;multiStored={};multiFetchedAt=0;multiFetchToken++;
  updateTrendButtons();loadMultiHistory();
}
function showTrend(id,module=S.moduleId){
  if(selectedTrend===id&&chartModule===Number(module)&&!historyError){
    $('trend').style.display='block';showView('live');$('trend').scrollIntoView({block:'start'});return;
  }
  chartModule=Number(module);
  if(!metadataFor(chartModule).some(x=>x.id===id))return;
  selectedTrend=id;multiPicked=[id];multiStored={};multiFetchToken++;
  setMultiPicker(false);$('multiSearch').value='';
  $('trend').style.display='block';showView('live');
  renderMultiPicker();checkHistoryStorage();setTrendRange('1h');
  $('trend').scrollIntoView({block:'start'});
}
const multiColors=['#51a7ff','#ffb74d','#56d6ad','#ce98ff','#ff6d8d','#ecdd7f','#71c4e3','#c0ccd8'];
function multiAvailable(){return metadataFor(chartModule).filter(x=>x.unit);}
function multiActiveIds(){
  const ids=[],available=new Set(metadataFor(chartModule).map(x=>x.id));
  for(const id of multiPicked){
    if(available.has(id)&&!ids.includes(id))ids.push(id);
    const target=pairSpecs[id]?.target;
    if(multiAutoTargets&&target&&available.has(target)&&!ids.includes(target))ids.push(target);
  }
  return ids;
}
function setMultiPicker(open,returnFocus=false){
  $('multiPicker').classList.toggle('open',open);
  $('multiPickerToggle').setAttribute('aria-expanded',String(open));
  $('multiPickerToggle').textContent=open?'Hide metrics':'+ Add metrics';
  if(open)renderMultiPicker();
  else{ $('multiSearch').blur();if(returnFocus)$('multiPickerToggle').focus(); }
}
function toggleMultiPicker(){setMultiPicker(!$('multiPicker').classList.contains('open'));}
function clearMultiMetrics(){multiPicked=selectedTrend?[selectedTrend]:[];multiSelectionChanged();}
function renderMultiPicker(){
  const q=($('multiSearch').value||'').trim().toLowerCase();
  const xs=multiAvailable().filter(x=>x.label.toLowerCase().includes(q)||x.id.toLowerCase().includes(q));
  $('multiSelectionCount').textContent=multiActiveIds().length+' of 8 chart series selected';
  $('multiOptions').innerHTML=xs.map(x=>'<label class="multiOption"><input type="checkbox" data-id="'+esc(x.id)+'" '+
    (multiPicked.includes(x.id)?'checked':'')+
    ' onchange="toggleMultiMetric(this.dataset.id,this.checked)"><span>'+esc(x.label)+
    (x.unit?' ('+esc(x.unit)+')':'')+'</span></label>').join('')||'<div class="note">No matching numeric measurements</div>';
}
function toggleMultiMetric(id,enabled){
  if(enabled&&!multiPicked.includes(id)){
    const prev=multiPicked.slice();multiPicked.push(id);
    if(multiActiveIds().length>8){multiPicked=prev;alert('Select up to eight series including targets.');return;}
  }else if(!enabled){multiPicked=multiPicked.filter(x=>x!==id);if(selectedTrend===id)selectedTrend=multiPicked[0]||null;}
  if(!multiPicked.length){closeTrend();return;}
  multiSelectionChanged();
}
function multiSelectionChanged(){
  if(multiActiveIds().length>8){$('multiPairTargets').checked=false;multiAutoTargets=false;alert('Automatic targets exceed eight series; target overlay disabled.');}
  multiStored={};multiFetchedAt=0;multiFetchToken++;renderMultiPicker();loadMultiHistory();
}
function multiData(){
  const meta=new Map(metadataFor(chartModule).map(x=>[x.id,x]));
  return multiActiveIds().map((id,index)=>({module:chartModule,id,label:meta.get(id)?.label||id,unit:meta.get(id)?.unit||'',
    color:multiColors[index],points:multiStored[id]?.points||[]}));
}
function drawTrend(){
  if(!selectedTrend)return;
  const series=multiData(),moduleName=modulesCatalog.find(x=>Number(x.id)===chartModule)?.name||'Module '+chartModule;
  $('trendTitle').textContent=moduleName+' · Saved history';
  $('trendSubtitle').textContent=trendRange+' · microSD records only';
  $('trendLegend').textContent=series.map(s=>s.label+(s.unit?' ('+s.unit+')':'')).join(' · ');
  $('trendPlot').setAttribute('aria-busy',String(historyLoading));
  if(historyError){$('trendPlot').innerHTML='';$('trendFoot').textContent='History unavailable: '+historyError;return;}
  if(historyLoading){
    $('trendPlot').innerHTML='<div class="historyLoading" role="status"><b>Loading saved microSD measurements…</b><span class="note">'+historyLoadedCount+' of '+series.length+' series read. This can take several seconds; the chart appears automatically.</span></div>';
    $('trendFoot').textContent='Live comparison timing does not affect saved charts.';return;
  }
  $('trendPlot').innerHTML=T1NData.renderHistorySvg(series,{width:Math.max(320,$('trendPlot').clientWidth||780)});
  const count=series.reduce((n,s)=>n+s.points.length,0),truncated=Object.values(multiStored).some(x=>x.truncated);
  $('trendFoot').textContent=count+' saved records'+(truncated?' · Response limited to newest records; choose a shorter range for detail.':'')+
    ' · Changes recorded at least 10 seconds apart; unchanged values every 30 minutes. Shading shows saved minimum/maximum; gaps remain gaps.';
}
async function loadMultiHistory(force=false){
  if(!selectedTrend)return;
  const ids=multiActiveIds(),module=chartModule,range=trendRange,token=++multiFetchToken;
  historyLoading=true;historyLoadedCount=0;historyError='';multiStored={};multiFetchedAt=Date.now();drawTrend();
  const loading=(async()=>{
    try{
      const loaded={};
      for(const id of ids){
        if(token!==multiFetchToken)return;
        loaded[id]=await client.history({module,id,range,force});
        if(token===multiFetchToken){historyLoadedCount++;drawTrend();}
      }
      if(token!==multiFetchToken)return;
      multiStored=loaded;historyLoading=false;drawTrend();
    }catch(e){
      if(token!==multiFetchToken)return;
      multiStored={};historyLoading=false;historyError=e.message;drawTrend();
    }
  })();
  historyLoadPromise=loading;await loading;
}
function multiDownload(name,body,mime){
  const blob=new Blob([body],{type:mime}),url=URL.createObjectURL(blob),a=document.createElement('a');
  a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();setTimeout(()=>URL.revokeObjectURL(url),10000);
}
function multiFilename(ext){return 't1n_module'+chartModule+'_history_'+trendRange+'.'+ext;}
function exportMultiCSV(){
  if(historyLoading||historyError||!multiData().some(s=>s.points.length)){alert('No available saved records to export.');return;}
  multiDownload(multiFilename('csv'),T1NData.exportCsv(multiData()),'text/csv;charset=utf-8');
}
function exportMultiSVG(){
  if(historyLoading||historyError||!multiData().some(s=>s.points.length)){alert('No available saved records to export.');return;}
  multiDownload(multiFilename('svg'),T1NData.renderHistorySvg(multiData()),'image/svg+xml;charset=utf-8');
}
document.addEventListener('click',e=>{let box=e.target.closest('[data-metric-id]');if(box&&box.closest('#live'))showTrend(box.dataset.metricId);});
function render(){
  byId=Object.fromEntries(items.map(x=>[x.id,x]));
  $('keyCards').innerHTML=renderItems(keyItems());$('allCards').innerHTML=renderItems(items);
  $('moduleName').textContent=S.module||'Unknown module';$('liveModule').textContent=S.module||'Unknown module';
  $('faultModule').textContent=S.module||'Unknown module';$('addr').textContent=S.address||'--';
  $('pin').textContent=S.obdPin||'--';$('relay').textContent=S.relay?'K'+S.relay:'--';
  $('relayGpio').textContent=S.relayGpio===255?'--':S.relayGpio;
  const variant=(S.moduleId===2||S.moduleId===1)&&S.variant;
  $('variantLine').style.display=variant?'block':'none';$('variantId').textContent=variant||'--';
  $('fw').textContent='Firmware '+(S.firmware||'unknown');
  const state=String(S.status||''),pending=connecting||(!S.connected&&(state.startsWith('CONNECTING')||state.startsWith('CONNECT QUEUED')||state.startsWith('AUTO RECONNECT')));
  $('linkState').textContent=scannerOnline?'Scanner online':'Scanner offline';
  $('dot').className='statusDot '+(scannerOnline?'ok':'bad');
  $('pollBadge').textContent=scannerOnline?'ECU '+(pending?'connecting':S.connected?'connected':'disconnected')+' · '+(S.polling?'polling enabled':'polling off'):'ECU status unavailable';
  $('err').textContent=scannerOnline?S.error||'':'';$('sensorError').textContent=sensorError;
  $('connectBtn').disabled=!scannerOnline||pending;
  $('connectBtn').innerHTML=pending?'<span class="spinner"></span>Connecting…':S.connected?'Reconnect':'Connect';
  $('disconnectBtn').disabled=!scannerOnline||!S.connected;
  $('absDtcNote').style.display=S.moduleId===1?'block':'none';$('absFaultLookup').style.display=S.moduleId===1?'block':'none';
  $('absReadBtn').disabled=S.moduleId===1;if(S.moduleId===1&&!$('absDtcResult').textContent)renderAbsFaultLookup();
  document.querySelectorAll('[data-module-id]').forEach(x=>x.classList.toggle('active',Number(x.dataset.moduleId)===S.moduleId));
  $('hzrFanPanel').style.display=S.moduleId===3?'block':'none';$('cr2TestPanel').style.display=S.moduleId===0?'block':'none';
  $('testOnlyEngine').style.display=S.moduleId===0?'none':'block';
  const feedback=['rpm','egr','boostDuty','boost','boostTarget','battery'];
  $('testFeedback').innerHTML=renderItems(items.filter(x=>feedback.includes(x.id)));
}
async function refresh(){
  if(refreshRunning)return;
  refreshRunning=true;clearTimeout(refreshTimer);
  try{
    const snapshot=await client.snapshot();
    S=snapshot.status;scannerOnline=true;sensorError=snapshot.sensorsError||'';snapshotReceivedMs=snapshot.receivedMs;
    items=snapshot.sensors.items;
    const metadata=new Map((metadataByModule[S.moduleId]||[]).map(x=>[x.id,x]));
    for(const x of items)metadata.set(x.id,{module:S.moduleId,id:x.id,label:x.label,unit:x.unit});
    metadataByModule[S.moduleId]=[...metadata.values()];
    let st=String(S.status||'');if(S.connected||(!st.startsWith('CONNECT')&&!S.busy))connecting=false;
    render();
    if(!modulesCatalog.length)await fetchModulesCatalog();
    if($('live').classList.contains('on'))await renderGroupCatalog();
    if(selectedTrend&&!historyLoading&&Date.now()-multiFetchedAt>60000)loadMultiHistory(true);
    if(Date.now()-historyStorageChecked>15000)await checkHistoryStorage();
    if($('logs').classList.contains('on'))await refreshLog();
    if($('faults').classList.contains('on')){
      try{const r=await fetch('/api/dtc',{cache:'no-store'});if(!r.ok)throw Error('HTTP '+r.status);$('faultText').textContent=await r.text();}
      catch(e){$('faultText').textContent='Fault results unavailable: '+e.message;}
    }
    if(S.moduleId===3&&$('tests').classList.contains('on')){
      try{const f=await client.request('/api/hzr/fan/state');$('hzrFanOutcome').textContent=f.message+(f.active?' (test running)':'');}catch(e){$('hzrFanOutcome').textContent='Fan status unavailable: '+e.message;}
    }
  }catch(e){
    scannerOnline=false;items=[];byId={};connecting=false;render();
    $('linkState').textContent='Scanner offline';$('dot').className='statusDot bad';
    $('sensorError').textContent='Scanner connection error: '+e.message;
    $('pollBadge').textContent='ECU status unavailable';
    if(selectedTrend){multiStored={};historyError='Scanner unreachable';historyLoading=false;multiFetchToken++;multiFetchedAt=0;drawTrend();}
    client.invalidateHistory();
  }finally{refreshRunning=false;refreshTimer=setTimeout(refresh,3000);}
}
async function post(u,b){let o={method:'POST'};if(b){o.headers={'Content-Type':'application/x-www-form-urlencoded'};o.body=new URLSearchParams(b)}let r=await fetch(u,o),t=await r.text();if(!r.ok)alert(t);setTimeout(refresh,200);return r.ok;}
async function fetchModulesCatalog(){
 try{const d=await client.request('/api/modules');modulesCatalog=d.modules||[];renderModuleCatalog();}catch(e){}
}
function renderModuleCatalog(){
 const box=$('moduleCatalog');if(!box)return;
 box.innerHTML=modulesCatalog.map(m=>`<div class="module ${Number(m.id)===S.moduleId?'active':''}" data-module-id="${Number(m.id)}">
 <h3>${esc(m.name)}</h3><div class="pin">OBD pin ${esc(m.obdPin)} · chip ${esc(m.channel)} · ${esc(m.address)}</div>
 <div class="note">${esc(m.liveCapability)} · ${m.pollable?'live data supported':'manual/status only'}</div>
 <div class="actions"><button onclick="selectModule(${Number(m.id)})">View saved history</button><button class="primary" onclick="connectActive(${Number(m.id)})">Connect manually</button></div></div>`).join('');
}
async function renderGroupCatalog(force=false){
 if(!force&&Date.now()-lastGroupUpdate<3500&&lastGroupModule===S.moduleId)return;
 lastGroupModule=S.moduleId;lastGroupUpdate=Date.now();
 const b=$('liveGroupCatalog');if(!b)return;
 try{
  const d=await client.request('/api/module/groups');
  if(lastGroupModule!==S.moduleId)return;
  const groups=d.groups||[];
  b.innerHTML=groups.length?groups.map(g=>`<div class="module">
   <h3>${esc(g.label||g.id)}</h3><div class="pin">${esc(g.id)}</div>
   <div class="note">${g.unsupported?'Unsupported by this ECU · ':''}${esc(g.problem||'')}
   ${g.lastMs?' · received on device':' · not yet read'}</div>
   <pre style="white-space:pre-wrap;word-break:break-word;font-size:12px">${esc(g.raw||'No response captured yet')}</pre>
   <div class="actions"><button onclick="readGroupOnce('${esc(g.id)}')" ${S.polling?'disabled':''}>Read once</button></div></div>`).join(''):
    '<div class="note">This module has no additional validated raw live-data groups yet.</div>';
 }catch(e){b.innerHTML='<div class="note">Unable to read group catalog</div>';}
}
async function readGroupOnce(group){
 if(S.polling){alert('Stop live polling before reading an individual group.');return;}
 const body=S.moduleId===2||S.moduleId===1?{group:group.split(' ').pop()}:{request:group};
 const ok=await post('/api/module/read',body);if(ok)await renderGroupCatalog(true);
}
function selectModule(id){const metrics=metadataFor(id).filter(x=>x.unit);if(metrics.length){showTrend(metrics[0].id,id);}else{closeTrend();showView('live');$('trend').style.display='block';$('trendTitle').textContent='Module '+id+' · Saved history';$('trendPlot').innerHTML='';$('trendFoot').textContent='No verified measurement definitions are available for this module.';}}async function connectActive(id=S.moduleId){connecting=true;render();const ok=await post('/api/module/connect',{id});if(!ok){connecting=false;render();}}async function disconnectActive(){connecting=false;await post('/api/disconnect')}async function pollOn(){await post('/api/poll/start')}async function pollOff(){await post('/api/poll/stop')}async function readDtc(){await post('/api/dtc/read');await refresh()}const acts=[["EGR",17],["Boost",18],["Intake shutoff",26],["Glow stage",30],["Fan control",27]];function buildActuators(){let h="";for(const a of acts){h+=`<div style="margin:12px 0;padding:10px;border:1px solid var(--line);border-radius:10px"><b>${a[0]}</b><div class="actions"><input id="v${a[1]}" type="number" min="0" max="100" step="1" value="50" style="width:88px;padding:10px;border-radius:8px;border:1px solid var(--line);background:#08111f;color:#fff"><button class="primary" onclick="act(${a[1]})">Send %</button><button onclick="releaseAct(${a[1]})">Release</button></div></div>`}$('actuatorControls').innerHTML=h;}function explainTest(text,ok,label,before){let box=$('testResult');box.classList.remove('testResultGood','testResultBad','testResultWait');let rejected=text.match(/NRC 0x([0-9A-F]+)/i);let accepted=/ECU accepted command/.test(text);let msg=ok&&accepted?'ECU ACCEPTED — '+label:rejected?'ECU REJECTED — '+label:'NO CONFIRMED ACTUATION — '+label;if(rejected){const causes={'31':'The ECU does not permit this request or these parameters in its current configuration/session. It is not evidence of a wiring failure.','22':'The ECU reports that operating conditions are not suitable.','33':'The ECU requires security authorization.','11':'The ECU does not support this diagnostic service.','12':'The requested function is unsupported.'};msg+='\n'+(causes[rejected[1].toUpperCase()]||'See the ECU negative-response code below.');}if(accepted){msg+='\nCommand accepted. Physical change is not yet verified. Check the relevant sensor or output. Some outputs have no observable PID.';if(before&&before.length)msg+='\nBaseline before command: '+before;}box.textContent=msg+'\n\n'+text;box.classList.add(accepted?'testResultGood':rejected?'testResultBad':'testResultWait');$('testExplain').textContent='A positive response confirms command acceptance only; verification requires a matching sensor change or an observable output.';}
async function hzrStatus(){let p=$('hzrFanStatus');p.textContent="Checking...";try{let r=await fetch('/api/hzr/fan/status',{method:'POST'});p.textContent=await r.text();if(!r.ok)p.textContent='ERROR: '+p.textContent;}catch(e){p.textContent='Request failed: '+e;}}
async function hzrStart(){if(!confirm('Experimental fan command: may override other HVAC outputs for 4 seconds. Vehicle parked? Hands clear of fan?'))return;let p=$('hzrFanOutcome');p.textContent='Sending fan command...';try{let r=await fetch('/api/hzr/fan/on',{method:'POST'});p.textContent=await r.text();if(!r.ok)p.textContent='ERROR: '+p.textContent;}catch(e){p.textContent='Command result unknown: '+e;}}
async function testCmd(name){let r=await fetch("/api/test",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams({name})}),t=await r.text();explainTest(t,r.ok,name,'');setTimeout(refresh,200);}async function act(id){let v=$('v'+id).value;let r=await fetch("/api/test/actuator",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams({id,value:v})}),t=await r.text();explainTest(t,r.ok,'Actuator '+id+' at '+v+'%', '');}async function releaseAct(id){let r=await fetch("/api/test/release",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams({id})}),t=await r.text();explainTest(t,r.ok,'Actuator '+id+' release','');}async function refreshLog(){if(logPaused)return;let sel=window.getSelection&&window.getSelection().toString();if(sel)return;try{$('activityLog').textContent=await(await fetch('/api/activity-log',{cache:'no-store'})).text()}catch(e){}}function toggleLog(){logPaused=!logPaused;$('pauseLogBtn').textContent=logPaused?'Resume':'Pause';if(!logPaused)refreshLog()}async function copyLog(){try{await navigator.clipboard.writeText($('activityLog').textContent)}catch(e){alert('Copy failed. Pause the log and select the text manually.')}}function exportLog(){window.location='/api/activity-log.txt'}async function clearActivityLog(){await post('/api/activity-log/clear');$('activityLog').textContent='';}buildActuators();refresh();</script></body></html>)HTML";}
#endif // T1N_USE_GZIP_PAGE
