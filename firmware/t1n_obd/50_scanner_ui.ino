#ifndef T1N_USE_GZIP_PAGE
String scannerPage(){return R"HTML(<!doctype html><html><head><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><meta name="theme-color" content="#08111f"><title>T1N Scanner</title><style>
:root{--bg:#07101c;--panel:#0d1928;--line:#24374f;--text:#eef6ff;--muted:#8ea2ba;--blue:#2b7fff;--green:#2fc477;--red:#ef5b62}*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:15px system-ui,-apple-system,Segoe UI,sans-serif}.wrap{max-width:1120px;margin:auto;padding:14px 14px 90px}.top{display:flex;justify-content:space-between;gap:12px;align-items:center;padding:5px 0 14px}.brand{font-size:22px;font-weight:850}.sub{color:var(--muted);font-size:12px}.statusDot{width:10px;height:10px;border-radius:50%;display:inline-block;background:#64748b;margin-right:7px}.statusDot.ok{background:var(--green);box-shadow:0 0 12px #2fc47788}.statusDot.bad{background:var(--red)}.statusDot.wait{background:#e7a93b;box-shadow:0 0 12px #e7a93b66}.nav{display:grid;grid-template-columns:repeat(6,1fr);gap:7px;background:#091523;border:1px solid var(--line);padding:6px;border-radius:14px;position:sticky;top:6px;z-index:10}.nav button{background:transparent;color:var(--muted);border:0;border-radius:10px;padding:10px 4px;font-weight:750}.nav button.on{background:#173252;color:#fff}.view{display:none}.view.on{display:block}.hero{margin:12px 0;background:linear-gradient(145deg,#10233a,#0c1725);border:1px solid var(--line);border-radius:16px;padding:16px}.heroRow{display:flex;justify-content:space-between;gap:12px;align-items:flex-start}.moduleName{font-size:23px;font-weight:850}.badge{display:inline-flex;align-items:center;border:1px solid var(--line);border-radius:999px;padding:5px 9px;color:#b9c8d8;font-size:12px}.actions{display:flex;gap:8px;flex-wrap:wrap;margin-top:14px}button{border:0;border-radius:10px;padding:10px 13px;background:#24415f;color:#fff;font-weight:800;cursor:pointer}.primary{background:var(--blue)!important}.cards{display:grid;grid-template-columns:repeat(auto-fit,minmax(165px,1fr));gap:9px;margin:10px 0}.metric{background:var(--panel);border:1px solid var(--line);border-radius:14px;padding:13px;min-height:116px}.metric{cursor:pointer;transition:border-color .15s,transform .15s}.metric:active{transform:scale(.99)}.metric.stale{opacity:.48}.metric.ok{border-color:#2f8f63}.metric.warn{border-color:#b88a36}.metric.bad{border-color:#c34f58}.metric .label{color:#9eb0c5;font-size:12px;min-height:30px}.metric .value{font-size:27px;font-weight:850;line-height:1.15;margin-top:4px;overflow-wrap:anywhere}.metric .unit{font-size:12px;color:#8296ad;margin-left:3px}.metric .imperial{font-size:12px;color:#a8bad0;margin-top:3px}.metric .expected{font-size:11px;color:#8296ad;line-height:1.35;margin-top:8px}.pairMetric{min-height:150px;grid-column:span 2}.pairGrid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:6px}.pairSide{background:#091421;border:1px solid #1b2d42;border-radius:10px;padding:9px}.pairTag{font-size:10px;letter-spacing:.08em;color:#71869e;font-weight:850}.pairValue{font-size:22px;font-weight:850;line-height:1.15;margin-top:3px;overflow-wrap:anywhere}.compare{font-size:12px;font-weight:750;margin-top:9px;color:#a8bad0}.metric.ok .compare{color:#8fd6b3}.metric.warn .compare{color:#e4bf73}.metric.bad .compare{color:#f29aa0}.metric.ok .expected{color:#8fd6b3}.metric.warn .expected{color:#e4bf73}.metric.bad .expected{color:#f29aa0}.sectionTitle{font-size:17px;font-weight:850;margin:18px 2px 8px}.moduleGrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(240px,1fr));gap:10px}.module{background:var(--panel);border:1px solid var(--line);border-radius:15px;padding:14px}.module.active{border-color:#3b82f6;box-shadow:inset 0 0 0 1px #3b82f633}.module h3{margin:0 0 5px}.pin{font-size:24px;font-weight:850;color:#8dc0ff}.note{color:var(--muted);font-size:12px;line-height:1.45}.faultBox{white-space:pre-wrap;background:#050b12;border:1px solid var(--line);border-radius:13px;padding:13px;min-height:130px;font:13px ui-monospace,monospace;color:#dce9f7}.notice{border:1px solid #6b5229;background:#251d11;color:#ffd999;border-radius:12px;padding:10px 12px;margin:10px 0}.error{color:#ff9ca2;font-size:13px;margin-top:8px}.footerLink{display:block;text-align:center;margin-top:22px;color:#91bfff;text-decoration:none}.tiny{font-size:11px;color:#71869e}.spinner{display:inline-block;width:14px;height:14px;border:2px solid #ffffff55;border-top-color:#fff;border-radius:50%;animation:spin .8s linear infinite;vertical-align:-2px;margin-right:6px}@keyframes spin{to{transform:rotate(360deg)}}.logBtns{display:flex;gap:8px;flex-wrap:wrap;margin:10px 0}@media(max-width:560px){.wrap{padding:10px 10px 80px}.nav{grid-template-columns:repeat(3,1fr)}.nav button{font-size:12px}.moduleName{font-size:20px}.metric .value{font-size:24px}}
 .trendBox{background:#0c1929;border:1px solid #315170;border-radius:15px;padding:15px;margin:12px 0}.trendBox svg{width:100%;height:auto;max-height:330px;min-height:160px;display:block}.trendTitle{font-size:17px;font-weight:800}.testResultGood{border-color:#248458;background:#0b231d}.testResultBad{border-color:#ac5158;background:#281518}.testResultWait{border-color:#997d38;background:#241f13}
.multiPicker{display:none;max-height:235px;overflow:auto;border:1px solid var(--line);background:#071321;border-radius:12px;padding:8px;margin-top:10px;grid-template-columns:repeat(auto-fit,minmax(205px,1fr));gap:5px}.multiPicker.open{display:grid}.multiOption{padding:7px;display:flex;align-items:center;gap:8px;font-size:12px;border-radius:7px}.multiOption:has(input:checked){background:#173252}.multiOption input{accent-color:#51a7ff}.multiLegend{display:flex;flex-wrap:wrap;gap:9px;margin:9px 0}.multiLegend span{font-size:12px;padding:5px 8px;background:#11253a;border-radius:8px}.multiControls{display:flex;gap:8px;flex-wrap:wrap;align-items:center}.multiControls input[type="search"]{max-width:215px;width:100%;background:#081626;color:#fff;border:1px solid #36506d;padding:10px;border-radius:8px}.multiControls label{font-size:13px;color:#c7d9ec;display:flex;align-items:center;gap:4px}.multiControls input[type="checkbox"]{accent-color:#51a7ff}.trendBox svg.multiGraph{width:100%;height:auto;max-height:320px;min-height:190px;display:block;margin-top:10px}.multiHint{font-size:12px;color:#9aaec4;margin-top:6px}
</style></head><body><div class="wrap"><div class="top"><div><div class="brand">T1N Scanner</div><div class="sub">2003 Sprinter diagnostic scanner · <span id="fw">...</span></div></div><div class="badge"><span id="dot" class="statusDot"></span><span id="linkState">Disconnected</span></div></div>
<div class="nav"><button class="on" data-v="overview">Overview</button><button data-v="live">Live Data</button><button data-v="faults">Fault Codes</button><button data-v="tests">Tests</button><button data-v="modules">Modules</button><button data-v="logs">Logs</button></div>
<section id="overview" class="view on"><div class="hero"><div class="heroRow"><div><div class="sub">ACTIVE MODULE</div><div id="moduleName" class="moduleName">Engine / CR2</div><div class="sub">Address <span id="addr">0x12</span> · OBD pin <b id="pin">7</b> · TLIN channel <b id="relay">K2</b> / RX GPIO <b id="relayGpio">37</b></div><div id="variantLine" class="sub" style="display:none">ECU variant <b id="variantId">--</b></div></div><span id="pollBadge" class="badge">Live data off</span></div><div id="err" class="error"></div><div class="actions"><button id="connectBtn" class="primary" onclick="connectActive()">Connect</button><button id="disconnectBtn" onclick="disconnectActive()">Disconnect</button><button onclick="showView('modules')">Choose Module</button></div></div><div class="sectionTitle">Key data</div><div id="keyCards" class="cards"></div><div class="note">Metric and imperial values are shown together where a useful conversion exists. Actual/target pairs share one card with a live delta. Pair-card colors use conservative tracking tolerances for diagnosis; they are not Mercedes fault-code thresholds.</div></section>
<section id="live" class="view"><div class="sectionTitle">Live Data · <span id="liveModule">...</span></div><div class="actions"><button class="primary" onclick="pollOn()">Start live data</button><button onclick="pollOff()">Stop</button></div><div class="note">Tap any measurement to view its recent history. Tap a measurement to open a chart, then use Add metrics to overlay other values. Live samples are kept in this browser while the page remains open.</div><div id="trend" class="trendBox" style="display:none"><div class="heroRow"><div><div id="trendTitle" class="trendTitle">History</div><div id="trendSubtitle" class="sub"></div></div><button onclick="closeTrend()">Close</button></div><div class="actions"><button class="primary" onclick="toggleMultiPicker()">+ Add metrics</button><button onclick="exportMultiCSV()">Export CSV</button><button onclick="exportMultiSVG()">Export SVG</button><button onclick="clearMultiMetrics()">Reset chart</button></div>
<div class="multiControls" style="margin-top:10px"><input id="multiSearch" type="search" placeholder="Find a measurement..." aria-label="Search chart measurements" oninput="renderMultiPicker()"><label><input id="multiPairTargets" type="checkbox" checked onchange="multiAutoTargets=this.checked;multiSelectionChanged()">Include target automatically</label></div>
<div id="multiPicker" class="multiPicker" aria-label="Add or remove measurements"></div>
<div class="actions" id="historyRanges">
<button onclick="setTrendRange('live')">Live</button>
<button onclick="setTrendRange('1h')">1 hour</button>
<button onclick="setTrendRange('24h')">24 hours</button>
<button onclick="setTrendRange('7d')">7 days</button>
<button onclick="setTrendRange('30d')">30 days</button>
<button onclick="setTrendRange('all')">All</button>
</div><svg id="trendPlot" viewBox="0 0 780 280" role="img" aria-label="Recent measurement history"></svg><div id="trendLegend" class="note"></div><div class="note" id="trendFoot"></div><div class="note" id="historyStorage">Checking on-device storage…</div></div><div id="allCards" class="cards"></div><div class="sectionTitle">ECU diagnostic groups / raw responses</div><div class="note">Raw frames are preserved for every captured group. Unlabelled bytes are displayed without guessing units or physical meaning. Stop live polling before reading one group manually.</div><div id="liveGroupCatalog" class="moduleGrid"></div></section>
<section id="faults" class="view"><div class="sectionTitle">Fault Codes · <span id="faultModule">...</span></div><div class="actions"><button class="primary" onclick="readDtc()">Read fault codes</button></div><div id="absDtcNote" class="notice" style="display:none">ABSBR901 fault definitions are decoded, but its CBF did not expose a verified fault-read request. This build will not guess or transmit an unverified ABS fault-memory command.</div><div id="faultText" class="faultBox">No fault-code read yet.</div><div class="note" style="margin-top:8px">Fault clearing and raw commands stay in Advanced so normal scanner use remains read-only.</div></section>
<section id="tests" class="view"><div id="hzrFanPanel" style="display:none"><div class="sectionTitle">A/C Auxiliary Fan · HZRKL</div><div class="module"><div class="note">OBD pin 15 · module 0x50 · fan control is experimental. Use ignition ON, vehicle parked, and keep hands away from the fan.</div><div class="actions"><button class="primary" onclick="hzrStatus()">Read Fan Status</button><button onclick="hzrStart()">Run Fan (4 seconds)</button></div><pre id="hzrFanStatus" class="faultBox" style="margin-top:10px;min-height:50px">Read fan status to check command output.</pre><div id="hzrFanOutcome" class="note"></div></div></div><div id="cr2TestPanel"><div class="sectionTitle">CR2 Active Tests</div><div class="sectionTitle" style="font-size:14px">Live ECU feedback</div><div id="testFeedback" class="cards"></div><div class="note">These values are read back from the ECU while live polling is active. A positive test response means the ECU accepted the command; it does not by itself prove a lamp, fan, valve, or actuator physically moved.</div><div id="testOnlyEngine" class="notice" style="display:none">These controls require Engine / CR2 on OBD pin 7.</div><div class="moduleGrid"><div class="module"><h3>Output tests</h3><div class="actions"><button onclick="testCmd('epc_on')">EPC lamp ON</button><button onclick="testCmd('epc_off')">EPC lamp OFF</button><button onclick="testCmd('glow_lamp_on')">Glow lamp ON</button><button onclick="testCmd('glow_lamp_off')">Glow lamp OFF</button><button onclick="testCmd('heater_on')">Aux heater ON</button><button onclick="testCmd('heater_off')">Aux heater OFF</button><button onclick="testCmd('fan_on')">Fan ON</button><button onclick="testCmd('fan_off')">Fan OFF</button><button onclick="testCmd('ac_cut_on')">A/C cutoff ON</button><button onclick="testCmd('ac_cut_off')">A/C cutoff OFF</button><button onclick="testCmd('ac_can_on')">A/C CAN ON</button><button onclick="testCmd('ac_can_off')">A/C CAN OFF</button></div></div><div class="module"><h3>Actuator override</h3><div class="note">Set a percentage, send it, then release control back to the ECU.</div><div id="actuatorControls"></div></div><div class="module"><h3>Compression test</h3><div class="actions"><button class="primary" onclick="testCmd('compression_lrr_on')">Start · LRR active</button><button class="primary" onclick="testCmd('compression_lrr_off')">Start · LRR disabled</button><button onclick="testCmd('compression_stop')">Stop</button></div></div></div><div class="sectionTitle">Last test result</div><div id="testResult" class="faultBox">No test sent yet.</div><div id="testExplain" class="note" style="margin-top:8px">The ECU response confirms acceptance or rejection. A physical outcome requires an appropriate live sensor changing.</div></div></section><section id="modules" class="view"><div class="sectionTitle">All ECUs on the four wired K-lines</div><div class="note">Multiple diagnostic addresses share a physical OBD pin; select the ECU, not the cable. Entries are based on successful vehicle or Autel communication, not assumed scan addresses.</div><div id="moduleCatalog" class="moduleGrid"></div><div class="notice">Pin 7: engine; pin 9: ABS + shift lever; pin 11: transmission; pin 15: HVAC + instrument cluster. ABS uses KWFB and its detailed live transport remains under investigation. EWM has a confirmed fault/status summary but no validated continuous sensor map.</div></section>
<section id="logs" class="view"><div class="sectionTitle">Diagnostic Activity Log</div><div class="note">Human-readable scanner events. Raw KWP traffic is still available in Advanced.</div><div class="logBtns"><button id="pauseLogBtn" onclick="toggleLog()">Pause</button><button onclick="copyLog()">Copy</button><button onclick="exportLog()">Export .txt</button><button onclick="clearActivityLog()">Clear</button></div><pre id="activityLog" class="faultBox" style="min-height:360px;max-height:65vh;overflow:auto">Loading...</pre></section><a class="footerLink" href="/advanced">Advanced tools, raw terminal, settings & firmware update →</a><div class="tiny" style="text-align:center;margin-top:8px">Active tests are only sent when you press a control.</div></div>
<script>let S={moduleId:0,connected:0,polling:0},modulesCatalog=[],lastGroupUpdate=0,lastGroupModule=-1,items=[],connecting=false,logPaused=false,history={},seenSamples={},selectedTrend=null,historyModule=-1,multiPicked=[],multiAutoTargets=true,multiStored={},multiFetchedAt=0,multiFetchToken=0,multiPickerSignature="",trendRange="live",historyStorageChecked=0;const $=x=>document.getElementById(x);document.querySelectorAll('.nav button').forEach(b=>b.onclick=()=>showView(b.dataset.v));function showView(v){document.querySelectorAll('.view').forEach(x=>x.classList.toggle('on',x.id===v));document.querySelectorAll('.nav button').forEach(x=>x.classList.toggle('on',x.dataset.v===v));}function esc(s){return String(s??'--').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));}function fmt(v,d){return Number(v).toFixed(d)}function imperial(x){if(!x||!x.valid)return'';let v=Number(x.value),u=x.unit||'';if(!Number.isFinite(v))return'';if(u==='°C')return fmt(v*9/5+32,1)+' °F';if(u==='km/h')return fmt(v*0.621371,1)+' mph';if(u==='km')return fmt(v*0.621371,0)+' mi';if(u==='bar')return fmt(v*14.5037738,x.id&&x.id.includes('lowFuel')?1:0)+' psi';if(u==='hPa'){if(x.id==='atmos')return fmt(v*0.0295299831,2)+' inHg';return fmt(v*0.0145037738,1)+' psi'}if(u==='mbar')return fmt(v*0.0145037738,1)+' psi';if(u==='Nm')return fmt(v*0.737562149,0)+' lb-ft';if(u==='mm')return fmt(v*0.0393700787,2)+' in';if(u==='kg/h')return fmt(v*2.20462262,1)+' lb/hr';if(u==='g/s')return fmt(v*7.93664144,1)+' lb/hr';if(u==='mg/stroke')return fmt(v*0.0154323584,3)+' gr/stroke';if(u==='mm³/stroke')return fmt(v*0.0000610237441,6)+' in³/stroke';return''}function primaryValue(x,cls='value'){let imp=imperial(x);return `<div class="${cls}">${esc(x&&x.valid?x.value:'--')}<span class="unit">${esc(x&&x.valid?x.unit:'')}</span></div>${imp?`<div class="imperial">${esc(imp)}</div>`:''}`}function card(x){let st=x.state||'neutral';return `<div class="metric ${x.stale?'stale':''} ${esc(st)}"><div class="label">${esc(x.label)}</div>${primaryValue(x)}<div class="expected">${esc(x.expected||'')}</div></div>`}const pairSpecs={lowFuel:{target:'lowFuelMin',title:'Low-side fuel pressure',targetLabel:'ECU MINIMUM',mode:'min',warn:0.15},boost:{target:'boostTarget',title:'Boost / MAP',mode:'delta',warn:200,bad:400,wp:.10,bp:.20},rail:{target:'railTarget',title:'Rail pressure',mode:'delta',warn:50,bad:100,wp:.10,bp:.20},gear:{target:'targetGear',title:'Gear',mode:'gear'},converterSlip:{target:'converterTargetSlip',title:'Converter slip',mode:'delta',warn:100,bad:250,wp:.15,bp:.30},shiftCurrentActual:{target:'shiftCurrentTarget',title:'Shift regulator current',mode:'delta',warn:75,bad:150,wp:.10,bp:.20},modCurrentActual:{target:'modCurrentTarget',title:'Modulating regulator current',mode:'delta',warn:75,bad:150,wp:.10,bp:.20}};let byId={};const pairedTargets=new Set(Object.values(pairSpecs).map(s=>s.target));function impDelta(unit,v,id){if(unit==='bar')return fmt(v*14.5037738,1)+' psi';if(unit==='hPa'||unit==='mbar')return fmt(v*0.0145037738,1)+' psi';if(unit==='Nm')return fmt(v*0.737562149,0)+' lb-ft';if(unit==='km/h')return fmt(v*0.621371,1)+' mph';return''}function pairEval(a,t,s){if(!a||!t||!a.valid||!t.valid||a.stale||t.stale)return{state:'neutral',text:'Waiting for fresh comparison data'};if(s.mode==='gear'){let shifting=(byId.upshift&&byId.upshift.value==='Active')||(byId.downshift&&byId.downshift.value==='Active');if(a.value===t.value)return{state:'ok',text:'Actual matches target'};return shifting?{state:'neutral',text:'Target differs while a shift is in progress'}:{state:'warn',text:'Actual gear differs from target'}}let av=Number(a.value),tv=Number(t.value);if(!Number.isFinite(av)||!Number.isFinite(tv))return{state:'neutral',text:'Comparison unavailable'};let d=av-tv;if(s.mode==='min'){let imp=impDelta(a.unit,d,a.id);if(d<0)return{state:'bad',text:`Below ECU minimum by ${fmt(Math.abs(d),3)} ${a.unit}${imp?' / '+impDelta(a.unit,Math.abs(d),a.id):''}`};if(d<s.warn)return{state:'warn',text:`Only ${fmt(d,3)} ${a.unit}${imp?' / '+imp:''} above ECU minimum`};return{state:'ok',text:`Margin +${fmt(d,3)} ${a.unit}${imp?' / +'+imp:''}`}}let ad=Math.abs(d),wt=Math.max(s.warn||0,Math.abs(tv)*(s.wp||0)),bt=Math.max(s.bad||0,Math.abs(tv)*(s.bp||0)),pct=Math.abs(tv)>0.0001?ad/Math.abs(tv)*100:null,imp=impDelta(a.unit,d,a.id),sign=d>=0?'+':'';let txt=`Δ ${sign}${fmt(d,Math.abs(d)<10?2:0)} ${a.unit}${imp?' / '+(d>=0?'+':'')+imp:''}${pct!==null?' · '+fmt(pct,1)+'%':''}`;return{state:ad>bt?'bad':ad>wt?'warn':'ok',text:txt}}function pairCard(a,t,s){let ev=pairEval(a,t,s),stale=(a&&a.stale)||(t&&t.stale);return `<div class="metric pairMetric ${stale?'stale':''} ${ev.state}"><div class="label">${esc(s.title)}</div><div class="pairGrid"><div class="pairSide"><div class="pairTag">ACTUAL</div>${primaryValue(a,'pairValue')}</div><div class="pairSide"><div class="pairTag">${esc(s.targetLabel||'TARGET')}</div>${primaryValue(t,'pairValue')}</div></div><div class="compare">${esc(ev.text)}</div></div>`}function renderItems(xs){return xs.filter(x=>!pairedTargets.has(x.id)).map(x=>{let s=pairSpecs[x.id],t=s?byId[s.target]:null;return `<div data-metric-id="${esc(x.id)}">${s&&t?pairCard(x,t,s):card(x)}</div>`}).join('')}function keyItems(){let ids=S.moduleId===0?['rpm','speed','coolant','battery','lowFuel','boost','rail']:S.moduleId===2?['temp','gear','converterSlip','shiftCurrentActual','modCurrentActual','speed','battery']:S.moduleId===1?['wheelFL','wheelFR','wheelRL','wheelRR','voltage','brakeLamp']:S.moduleId===5?['kiFuel','kiOutside','kiDimmer','kiSpeed','kiSender']:S.moduleId===3?['g2164_b0','g2164_b1','g2164_b2']:[];return ids.map(id=>byId[id]).filter(Boolean)}function rememberItems(){
  if(historyModule!==S.moduleId){
    history={};seenSamples={};selectedTrend=null;historyModule=S.moduleId;
    multiPicked=[];trendRange="live";multiStored={};multiFetchedAt=0;multiFetchToken++;multiPickerSignature="";
    $('trend').style.display='none';
  }
  const now=Date.now();
  for(const x of items){
    if(!x.valid||x.stale)continue;
    const value=Number(String(x.value??'').trim());
    const sample=Number(x.sampleMs||0);
    if(!Number.isFinite(value)||!sample||seenSamples[x.id]===sample)continue;
    seenSamples[x.id]=sample;
    const age=Math.max(0,Math.min(30000,Number(x.ageMs)||0));
    let time=now-age;
    let points=history[x.id]||(history[x.id]=[]);
    if(points.length&&time<=points[points.length-1][0])time=points[points.length-1][0]+1;
    points.push([time,value]);
    while(points.length>700)points.shift();
  }
  // Draw after render() updates byId so labels and target pairs are current.
}

function closeTrend(){selectedTrend=null;multiPicked=[];multiStored={};multiFetchToken++;$('trend').style.display='none';$('multiPicker').classList.remove('open');}
async function checkHistoryStorage(){
  historyStorageChecked=Date.now();
  try{
    const d=await(await fetch('/api/history/status',{cache:'no-store'})).json();
    const total=Math.round((d.capacityBytes||0)/1024),used=Math.round((d.usedBytes||0)/1024);
    $('historyStorage').textContent=d.error?'Flash history: '+d.error+' · browser live data remains available':
      'ESP32 history: '+used+'/'+total+' KiB · '+(d.timeReady?'clock synchronized':'waiting for time');
  }catch(e){$('historyStorage').textContent='Could not check ESP32 history';}
}
async function syncHistoryClock(){
  try{await fetch('/api/history/clock',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({epoch:Math.floor(Date.now()/1000)})});}catch(e){}
}
function updateTrendButtons(){
  $('historyRanges').querySelectorAll('button').forEach(b=>{
    const m=(b.getAttribute('onclick')||'').match(/'([^']+)'/);
    b.classList.toggle('primary',!!m&&m[1]===trendRange);
  });
}
function setTrendRange(range){
  trendRange=range;multiStored={};multiFetchedAt=0;multiFetchToken++;
  updateTrendButtons();drawTrend();
  if(range!=='live')loadMultiHistory();
}
function showTrend(id){
  const x=byId[id];
  if(!x)return;
  selectedTrend=id;multiPicked=[id];multiStored={};multiFetchToken++;
  $('multiPicker').classList.remove('open');$('multiSearch').value='';
  $('trend').style.display='block';multiPickerSignature='';
  renderMultiPicker();checkHistoryStorage();setTrendRange('live');
  $('trend').scrollIntoView({behavior:'smooth',block:'nearest'});
}
const multiColors=['#51a7ff','#ffb74d','#56d6ad','#ce98ff','#ff6d8d','#ecdd7f','#71c4e3','#c0ccd8'];
const metricNumeric=x=>x&&(x.unit||x.valid&&String(x.value??'').trim()!==''&&Number.isFinite(Number(x.value)));
function multiAvailable(){return items.filter(metricNumeric);}
function multiActiveIds(){
  const ids=[];
  for(const id of multiPicked){
    if(byId[id]&&!ids.includes(id))ids.push(id);
    const target=pairSpecs[id]?.target;
    if(multiAutoTargets&&target&&byId[target]&&!ids.includes(target))ids.push(target);
  }
  return ids;
}
function multiName(id){return byId[id]?.label||id;}
function toggleMultiPicker(){const picker=$('multiPicker');picker.classList.toggle('open');renderMultiPicker();}
function clearMultiMetrics(){
  multiPicked=selectedTrend?[selectedTrend]:[];
  multiSelectionChanged();
}
function renderMultiPickerIfNeeded(){
  const signature=S.moduleId+':'+items.map(x=>x.id+':'+x.unit).join(',');
  if(signature!==multiPickerSignature){multiPickerSignature=signature;renderMultiPicker();}
}
function renderMultiPicker(){
  const q=($('multiSearch').value||'').trim().toLowerCase();
  const xs=multiAvailable().filter(x=>x.label.toLowerCase().includes(q)||x.id.toLowerCase().includes(q));
  $('multiPicker').innerHTML=xs.map(x=>'<label class="multiOption"><input type="checkbox" data-id="'+esc(x.id)+'" '+
    (multiPicked.includes(x.id)?'checked':'')+
    ' onchange="toggleMultiMetric(this.dataset.id,this.checked)"><span>'+esc(x.label)+
    (x.unit?' ('+esc(x.unit)+')':'')+'</span></label>').join('')||'<div class="note">No matching numeric readings</div>';
}
function toggleMultiMetric(id,enabled){
  if(enabled&&!multiPicked.includes(id)){
    const prev=multiPicked.slice();
    multiPicked.push(id);
    if(multiActiveIds().length>8){
      multiPicked=prev;
      alert('Up to 8 plotted lines including automatic targets. Remove one first.');
    }
  }else if(!enabled){
    multiPicked=multiPicked.filter(x=>x!==id);
    if(selectedTrend===id)selectedTrend=multiPicked[0]||null;
  }
  if(!selectedTrend&&!multiPicked.length){
    closeTrend();return;
  }
  multiSelectionChanged();
}
function multiSelectionChanged(){
  if(multiActiveIds().length>8){
    $('multiPairTargets').checked=false;
    multiAutoTargets=false;
    alert('Automatic targets exceed 8 plotted lines. Target overlay was disabled.');
  }
  multiStored={};multiFetchedAt=0;multiFetchToken++;
  renderMultiPicker();drawTrend();
  if(trendRange!=='live')loadMultiHistory();
}
function multiBrowserPoints(id){
  const sec={'1h':3600,'24h':86400,'7d':604800,'30d':2592000};
  const cutoff=trendRange==='live'?-Infinity:Date.now()/1000-(sec[trendRange]??Infinity);
  return (history[id]||[]).filter(p=>p[0]/1000>=cutoff)
    .map(p=>({t:p[0]/1000,avg:p[1],min:p[1],max:p[1],source:'browser'}));
}
function multiSeries(id){
  if(trendRange==='live')return multiBrowserPoints(id);
  if(multiStored[id]?.length)return multiStored[id]
    .filter(p=>Number.isFinite(Number(p.t))&&Number.isFinite(Number(p.avg)))
    .map(p=>({...p,source:'flash'}));
  return multiBrowserPoints(id);
}
async function loadMultiHistory(){
  if(!selectedTrend||trendRange==='live')return;
  const ids=multiActiveIds(),mod=S.moduleId,range=trendRange,token=++multiFetchToken;
  multiFetchedAt=Date.now();
  for(const id of ids){
    if(token!==multiFetchToken||S.moduleId!==mod||trendRange!==range)return;
    try{
      const r=await fetch('/api/history/data?module='+mod+'&id='+encodeURIComponent(id)+'&range='+range,{cache:'no-store'});
      if(!r.ok)throw Error('HTTP '+r.status);
      const d=await r.json();
      if(token!==multiFetchToken||S.moduleId!==mod||trendRange!==range)return;
      multiStored[id]=(d.points||[]).sort((a,b)=>Number(a.t)-Number(b.t));
      drawTrend();
    }catch(e){if(token!==multiFetchToken)return;multiStored[id]=[];}
  }
}
function multiData(){
  return multiActiveIds().map((id,index)=>({id,label:multiName(id),unit:byId[id]?.unit||'',
    color:multiColors[index%multiColors.length],points:multiSeries(id),
    target:multiAutoTargets&&Object.values(pairSpecs).some(x=>x.target===id)&&!multiPicked.includes(id)}));
}
function axisFormat(v){
  if(!Number.isFinite(v))return '--';
  if(Math.abs(v)>=10000)return v.toFixed(0);
  if(Math.abs(v)>=100)return Number(v.toFixed(1)).toString();
  if(Math.abs(v)>=1)return Number(v.toFixed(2)).toString();
  return Number(v.toPrecision(3)).toString();
}
function drawTrend(){
  if(!selectedTrend)return;
  const series=multiData();
  const svg=$('trendPlot'),legend=$('trendLegend'),foot=$('trendFoot');
  $('trendTitle').textContent=series.length===1?series[0].label:'Measurement history · '+series.length+' lines';
  $('trendSubtitle').textContent=(trendRange==='live'?'Live':'Saved / browser')+' · '+series.map(s=>{
    const v=byId[s.id];
    return s.label+': '+(v?.valid?v.value+' '+(v.unit||''):'--');
  }).join('  |  ');
  legend.innerHTML=series.map(s=>'<span style="display:inline-flex;align-items:center;gap:5px"><b style="color:'+s.color+'">●</b>'+esc(s.label)+(s.target?' (target)':'')+' ('+esc(s.unit)+')</span>').join(' ');
  const valid=[];
  for(const s of series)for(const p of s.points){
    const t=Number(p.t),v=Number(p.avg);
    if(Number.isFinite(t)&&Number.isFinite(v))valid.push({t,v,id:s.id});
  }
  if(!valid.length){
    svg.innerHTML='<text x="190" y="120" fill="#9fb4cb">Waiting for selected live readings…</text>';
    foot.textContent='Select a measurement and enable live polling to collect samples.';
    return;
  }
  const scales={},units=[];
  for(const s of series){
    const unit=s.unit||'unitless';
    for(const p of s.points){
      const lo=Number(p.min),hi=Number(p.max),value=Number(p.avg);
      if(!Number.isFinite(value))continue;
      if(!scales[unit]){scales[unit]={min:Infinity,max:-Infinity,color:s.color};units.push(unit);}
      const k=scales[unit];
      k.min=Math.min(k.min,Number.isFinite(lo)?lo:value,value);
      k.max=Math.max(k.max,Number.isFinite(hi)?hi:value,value);
    }
  }
  const X0=176,X1=762,Y0=43,Y1=230;
  const minTime=Math.min(...valid.map(p=>p.t)),maxTime=Math.max(...valid.map(p=>p.t));
  const x=t=>maxTime===minTime?(X0+X1)/2:X0+(X1-X0)*(t-minTime)/(maxTime-minTime);
  const y=(v,k)=>{
    const a=scales[k];
    return a.min===a.max?(Y0+Y1)/2:Y1-(Y1-Y0)*(v-a.min)/(a.max-a.min);
  };
  let content='<rect x="'+X0+'" y="'+Y0+'" width="'+(X1-X0)+'" height="'+(Y1-Y0)+'" fill="#081523" stroke="#365473"/>';
  for(let i=1;i<4;i++){
    const yy=Y0+(Y1-Y0)*i/4;
    content+='<line x1="'+X0+'" y1="'+yy.toFixed(1)+'" x2="'+X1+'" y2="'+yy.toFixed(1)+'" stroke="#30455b" stroke-dasharray="3 5"/>';
  }
  content+='<text x="10" y="25" fill="#9db2cc" font-size="12">Unit · MIN – MAX</text>';
  for(let i=0;i<units.length;i++){
    const unit=units[i],s=scales[unit],yy=47+i*23;
    content+='<circle cx="13" cy="'+(yy+5)+'" r="4" fill="'+s.color+'"/>'+
      '<text x="22" y="'+(yy+9)+'" fill="'+s.color+'" font-size="12">'+esc(unit)+'</text>'+
      '<text x="169" y="'+(yy+9)+'" fill="#d7e4f3" font-size="12" text-anchor="end">'+
      esc(axisFormat(s.min)+' – '+axisFormat(s.max))+'</text>';
  }
  for(const s of series){
    const unit=s.unit||'unitless';
    if(!scales[unit])continue;
    const pts=s.points.filter(p=>Number.isFinite(Number(p.t))&&Number.isFinite(Number(p.avg)));
    if(!pts.length)continue;
    const coords=pts.map(p=>x(Number(p.t)).toFixed(1)+','+y(Number(p.avg),unit).toFixed(1)).join(' ');
    content+='<polyline points="'+coords+'" fill="none" stroke="'+s.color+'" stroke-width="2.6" stroke-linejoin="round" stroke-linecap="round"'+(s.target?' stroke-dasharray="7 4"':'')+'/>';
    if(pts.length<=45)for(const p of pts){
      content+='<circle cx="'+x(Number(p.t)).toFixed(1)+'" cy="'+y(Number(p.avg),unit).toFixed(1)+'" r="2.6" fill="'+s.color+'"/>';
    }
  }
  const fmtTime=t=>esc(new Date(t*1000).toLocaleString());
  content+='<text x="'+X0+'" y="249" fill="#a6bbd2" font-size="11">'+fmtTime(minTime)+'</text>'+
    '<text x="'+X1+'" y="249" text-anchor="end" fill="#a6bbd2" font-size="11">'+fmtTime(maxTime)+'</text>';
  content+='<text x="'+X0+'" y="273" fill="#839ab6" font-size="11">Each unit uses its exact recorded low/high as its vertical bounds</text>';
  svg.innerHTML=content;
  const flash=series.some(s=>s.points.some(p=>p.source==='flash'));
  foot.textContent=series.length+' lines · '+valid.length+' observations · '+
    (flash?'ESP32 stored history (15-minute ranges may be averages)':'browser live samples')+
    (units.length>1?' · different units have independent vertical scales':'');
}
function multiRows(){
  const rows=[];
  for(const s of multiData())for(const p of s.points){
    if(!Number.isFinite(Number(p.t))||!Number.isFinite(Number(p.avg)))continue;
    rows.push({timestamp:new Date(Number(p.t)*1000).toISOString(),id:s.id,label:s.label,unit:s.unit,
      value:Number(p.avg),minimum:p.min==null?'':Number(p.min),maximum:p.max==null?'':Number(p.max),
      source:p.source||'browser'});
  }
  return rows.sort((a,b)=>a.timestamp.localeCompare(b.timestamp)||a.id.localeCompare(b.id));
}
function multiDownload(name,body,mime){
  const blob=new Blob([body],{type:mime}),url=URL.createObjectURL(blob),a=document.createElement('a');
  a.href=url;a.download=name;document.body.appendChild(a);a.click();a.remove();
  setTimeout(()=>URL.revokeObjectURL(url),10000);
}
function multiFilename(ext){
  return 't1n_'+(S.module||'module').replace(/[^a-z0-9]+/gi,'_').toLowerCase()+
    '_chart_'+trendRange+'_'+new Date().toISOString().replace(/[:.]/g,'-')+'.'+ext;
}
function exportMultiCSV(){
  const rows=multiRows();
  if(!rows.length){alert('No readings in this chart range to export.');return;}
  const quote=v=>'"'+String(v??'').replace(/"/g,'""')+'"';
  const csv=[['Timestamp UTC','Module','Metric ID','Metric','Unit','Value (avg if stored)','Minimum','Maximum','Source'].map(quote).join(',')];
  for(const p of rows)csv.push([p.timestamp,S.module,p.id,p.label,p.unit,p.value,p.minimum,p.maximum,p.source].map(quote).join(','));
  multiDownload(multiFilename('csv'),'\uFEFF'+csv.join('\r\n')+'\r\n','text/csv;charset=utf-8');
}
function exportMultiSVG(){
  if(!multiRows().length){alert('No plotted readings to export.');return;}
  const svg=$('trendPlot').cloneNode(true);
  svg.setAttribute('xmlns','http://www.w3.org/2000/svg');
  svg.setAttribute('width','1170');svg.setAttribute('height','420');
  const title=document.createElementNS('http://www.w3.org/2000/svg','title');
  title.textContent='T1N '+(S.module||'')+' · '+trendRange+' · '+multiData().map(s=>s.label).join(' | ');
  svg.insertBefore(title,svg.firstChild);
  multiDownload(multiFilename('svg'),new XMLSerializer().serializeToString(svg),'image/svg+xml;charset=utf-8');
}
document.addEventListener('click',e=>{let box=e.target.closest('[data-metric-id]');if(box&&box.closest('#live'))showTrend(box.dataset.metricId);});
function render(){byId=Object.fromEntries(items.map(x=>[x.id,x]));$('keyCards').innerHTML=renderItems(keyItems());$('allCards').innerHTML=renderItems(items);$('moduleName').textContent=S.module;$('liveModule').textContent=S.module;$('faultModule').textContent=S.module;$('addr').textContent=S.address;$('pin').textContent=S.obdPin;$('relay').textContent=S.relay?('K'+S.relay):'--';$('relayGpio').textContent=S.relayGpio===255?'--':S.relayGpio;let showVariant=(S.moduleId===2||S.moduleId===1)&&S.variant;$('variantLine').style.display=showVariant?'block':'none';$('variantId').textContent=showVariant?S.variant:'--';$('fw').textContent='Firmware '+S.firmware;$('linkState').textContent=S.connected?'Connected':'Disconnected';$('dot').className='statusDot '+(S.connected?'ok':'bad');$('pollBadge').textContent=S.polling?'Live data running':'Live data off';$('err').textContent=S.error||'';let st=String(S.status||'');let cx=connecting||(!S.connected&&(st.startsWith('CONNECTING')||st.startsWith('CONNECT QUEUED')||st.startsWith('AUTO RECONNECT')));$('linkState').textContent=cx?'Connecting…':(S.connected?'Connected':'Disconnected');$('dot').className='statusDot '+(cx?'wait':(S.connected?'ok':'bad'));$('connectBtn').disabled=cx;$('connectBtn').innerHTML=cx?'<span class="spinner"></span>Connecting…':(S.connected?'Reconnect':'Connect');$('disconnectBtn').disabled=!S.connected;$('absDtcNote').style.display=S.moduleId===1?'block':'none';document.querySelectorAll('[data-module-id]').forEach(x=>x.classList.toggle('active',Number(x.dataset.moduleId)===S.moduleId));$('hzrFanPanel').style.display=S.moduleId===3?'block':'none';$('cr2TestPanel').style.display=S.moduleId===0?'block':'none';$('testOnlyEngine').style.display=S.moduleId===0?'none':'block';let ids=['rpm','egr','boostDuty','boost','boostTarget','battery'];$('testFeedback').innerHTML=renderItems(items.filter(x=>ids.includes(x.id)));}async function refresh(){try{S=await(await fetch('/api/status',{cache:'no-store'})).json();let st=String(S.status||'');if(S.connected||(!(st.startsWith('CONNECTING')||st.startsWith('CONNECT QUEUED')||st.startsWith('AUTO RECONNECT'))&&!S.busy))connecting=false;let d=await(await fetch('/api/sensors',{cache:'no-store'})).json();items=d.items||[];rememberItems();render();if(!modulesCatalog.length)fetchModulesCatalog();renderGroupCatalog();if(selectedTrend){renderMultiPickerIfNeeded();drawTrend();if(trendRange!=="live"&&Date.now()-multiFetchedAt>60000)loadMultiHistory();if(Date.now()-historyStorageChecked>15000)checkHistoryStorage();}if(S.moduleId===3){try{const f=await(await fetch("/api/hzr/fan/state",{cache:"no-store"})).json();$("hzrFanOutcome").textContent=f.message+(f.active?" (test running)":"");}catch(e){}}$('faultText').textContent=await(await fetch('/api/dtc',{cache:'no-store'})).text()}catch(e){$('linkState').textContent='UI offline';$('dot').className='statusDot bad';}}async function post(u,b){let o={method:'POST'};if(b){o.headers={'Content-Type':'application/x-www-form-urlencoded'};o.body=new URLSearchParams(b)}let r=await fetch(u,o),t=await r.text();if(!r.ok)alert(t);setTimeout(refresh,200);return r.ok;}
async function fetchModulesCatalog(){
 try{const r=await fetch('/api/modules',{cache:'no-store'});if(!r.ok)return;
 const d=await r.json();modulesCatalog=d.modules||[];renderModuleCatalog();}catch(e){}
}
function renderModuleCatalog(){
 const box=$('moduleCatalog');if(!box)return;
 box.innerHTML=modulesCatalog.map(m=>`<div class="module ${Number(m.id)===S.moduleId?'active':''}" data-module-id="${Number(m.id)}">
 <h3>${esc(m.name)}</h3><div class="pin">OBD pin ${esc(m.obdPin)} · chip ${esc(m.channel)} · ${esc(m.address)}</div>
 <div class="note">${esc(m.liveCapability)} · ${m.pollable?'live data supported':'manual/status only'}</div>
 <div class="actions"><button class="primary" onclick="selectModule(${Number(m.id)})">Select module</button></div></div>`).join('');
}
async function renderGroupCatalog(force=false){
 if(!force&&Date.now()-lastGroupUpdate<3500&&lastGroupModule===S.moduleId)return;
 lastGroupModule=S.moduleId;lastGroupUpdate=Date.now();
 const b=$('liveGroupCatalog');if(!b)return;
 try{
  const d=await(await fetch('/api/module/groups',{cache:'no-store'})).json();
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
async function selectModule(id){showView('overview');await post('/api/module/select',{id})}async function connectActive(){connecting=true;render();await post('/api/module/connect',{id:S.moduleId})}async function disconnectActive(){connecting=false;await post('/api/disconnect')}async function pollOn(){await post('/api/poll/start')}async function pollOff(){await post('/api/poll/stop')}async function readDtc(){await post('/api/dtc/read');await refresh()}const acts=[["EGR",17],["Boost",18],["Intake shutoff",26],["Glow stage",30],["Fan control",27]];function buildActuators(){let h="";for(const a of acts){h+=`<div style="margin:12px 0;padding:10px;border:1px solid var(--line);border-radius:10px"><b>${a[0]}</b><div class="actions"><input id="v${a[1]}" type="number" min="0" max="100" step="1" value="50" style="width:88px;padding:10px;border-radius:8px;border:1px solid var(--line);background:#08111f;color:#fff"><button class="primary" onclick="act(${a[1]})">Send %</button><button onclick="releaseAct(${a[1]})">Release</button></div></div>`}$('actuatorControls').innerHTML=h;}function explainTest(text,ok,label,before){let box=$('testResult');box.classList.remove('testResultGood','testResultBad','testResultWait');let rejected=text.match(/NRC 0x([0-9A-F]+)/i);let accepted=/ECU accepted command/.test(text);let msg=ok&&accepted?'ECU ACCEPTED — '+label:rejected?'ECU REJECTED — '+label:'NO CONFIRMED ACTUATION — '+label;if(rejected){const causes={'31':'The ECU does not permit this request or these parameters in its current configuration/session. It is not evidence of a wiring failure.','22':'The ECU reports that operating conditions are not suitable.','33':'The ECU requires security authorization.','11':'The ECU does not support this diagnostic service.','12':'The requested function is unsupported.'};msg+='\n'+(causes[rejected[1].toUpperCase()]||'See the ECU negative-response code below.');}if(accepted){msg+='\nCommand accepted. Physical change is not yet verified. Check the relevant sensor or output. Some outputs have no observable PID.';if(before&&before.length)msg+='\nBaseline before command: '+before;}box.textContent=msg+'\n\n'+text;box.classList.add(accepted?'testResultGood':rejected?'testResultBad':'testResultWait');$('testExplain').textContent='A positive response confirms command acceptance only; verification requires a matching sensor change or an observable output.';}
async function hzrStatus(){let p=$('hzrFanStatus');p.textContent="Checking...";try{let r=await fetch('/api/hzr/fan/status',{method:'POST'});p.textContent=await r.text();if(!r.ok)p.textContent='ERROR: '+p.textContent;}catch(e){p.textContent='Request failed: '+e;}}
async function hzrStart(){if(!confirm('Experimental fan command: may override other HVAC outputs for 4 seconds. Vehicle parked? Hands clear of fan?'))return;let p=$('hzrFanOutcome');p.textContent='Sending fan command...';try{let r=await fetch('/api/hzr/fan/on',{method:'POST'});p.textContent=await r.text();if(!r.ok)p.textContent='ERROR: '+p.textContent;}catch(e){p.textContent='Command result unknown: '+e;}}
async function testCmd(name){let r=await fetch("/api/test",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams({name})}),t=await r.text();explainTest(t,r.ok,name,'');setTimeout(refresh,200);}async function act(id){let v=$('v'+id).value;let r=await fetch("/api/test/actuator",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams({id,value:v})}),t=await r.text();explainTest(t,r.ok,'Actuator '+id+' at '+v+'%', '');}async function releaseAct(id){let r=await fetch("/api/test/release",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams({id})}),t=await r.text();explainTest(t,r.ok,'Actuator '+id+' release','');}async function refreshLog(){if(logPaused)return;let sel=window.getSelection&&window.getSelection().toString();if(sel)return;try{$('activityLog').textContent=await(await fetch('/api/activity-log',{cache:'no-store'})).text()}catch(e){}}function toggleLog(){logPaused=!logPaused;$('pauseLogBtn').textContent=logPaused?'Resume':'Pause';if(!logPaused)refreshLog()}async function copyLog(){try{await navigator.clipboard.writeText($('activityLog').textContent)}catch(e){alert('Copy failed. Pause the log and select the text manually.')}}function exportLog(){window.location='/api/activity-log.txt'}async function clearActivityLog(){await post('/api/activity-log/clear');$('activityLog').textContent='';}buildActuators();syncHistoryClock();refresh();refreshLog();setInterval(refresh,850);setInterval(refreshLog,1200);</script></body></html>)HTML";}
#endif // T1N_USE_GZIP_PAGE
