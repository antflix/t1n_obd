// NODE_PATH=<Playwright installation> node tests/web_dashboards.cjs
// Entirely local fixtures: never connects to a scanner or changes a diagnostic session.
'use strict';
const assert=require('node:assert/strict'),http=require('node:http'),fs=require('node:fs'),path=require('node:path');
const {chromium,webkit}=require('playwright');
const root=path.resolve(__dirname,'..');
let mode='normal',moduleId=0,connected=1,pending=0,writes=[],requests=[];
const t=1793300000;
const points=[{t:t-200,min:82,avg:85,max:88,count:3,tier:0},{t:t-20,min:89,avg:90,max:91,count:2,tier:0}];
const modules=[{id:0,name:'Engine CR2',address:'0x12',obdPin:7},{id:1,name:'ABS',address:'0x34',obdPin:9},{id:2,name:'EGS transmission',address:'0x20',obdPin:11},{id:3,name:'HVAC',address:'0x50',obdPin:15},{id:4,name:'EWM',address:'0x24',obdPin:9},{id:5,name:'Instrument cluster',address:'0x69',obdPin:15}];
const sensor=(id,value,unit='°C')=>({id,label:id,value:String(value),unit,valid:1,stale:mode==='stale'?1:0,ageMs:mode==='stale'?40000:100,sampleMs:1000,state:'neutral',expected:''});
function mobileHtml(){
 const src=fs.readFileSync(path.join(root,'firmware/t1n_obd/50_scanner_ui.ino'),'utf8');
 const match=src.match(/String scannerPage\(\)\{return R"HTML\(([\s\S]*?)\)HTML";\}/);
 assert(match,'scannerPage raw HTML asset missing');return match[1];
}
const server=http.createServer((req,res)=>{
 const u=new URL(req.url,'http://fixture');requests.push(u.pathname+u.search);if(req.method!=='GET')writes.push(req.method+' '+req.url);
 if(!u.pathname.startsWith('/api/')){
  const asset=u.pathname==='/t1n-data.js'?'t1n-data.js':u.pathname==='/pi'?'pi.html':null;
  res.setHeader('Content-Type',asset==='t1n-data.js'?'application/javascript':'text/html; charset=utf-8');
  res.end(asset?fs.readFileSync(path.join(root,'firmware/sdcard',asset)):mobileHtml());return;
 }
 if(mode==='offline'){res.destroy();return;}
 let code=200,data={};
 if(u.pathname==='/api/status')data={moduleId,module:modules[moduleId].name,connected,polling:connected,firmware:'TEST-FIXTURE',uptimeMs:2000,status:'Read-only fixture',error:''};
 else if(u.pathname==='/api/sensors'){
  if(mode==='sensorError'){code=503;data={error:'Sensor response unavailable'};}
  else data={moduleId,module:modules[moduleId].name,items:moduleId===2?[sensor('temp',50)]:[sensor('rpm',812,'rpm'),sensor('coolant',85),sensor('battery',14,'V'),sensor('rail',300,'bar'),sensor('railTarget',302,'bar'),sensor('boost',1000,'hPa'),sensor('boostTarget',1002,'hPa'),sensor('speed',0,'km/h')]};
 }
 else if(u.pathname==='/api/history/data'){
  if(mode==='pending'&&pending++<2){code=202;data={ok:false,loading:true,code:'HISTORY_LOADING',error:'SD query loading'};}
  else if(mode==='historyError'||mode==='storageError'){code=503;data={ok:false,code:'HISTORY_READ_FAILED',error:'Card read failed',points:[]};}
  else data={ok:true,schemaVersion:1,storage:'microSD',module:Number(u.searchParams.get('module')),id:u.searchParams.get('id'),range:u.searchParams.get('range'),points:mode==='empty'?[]:points,count:mode==='empty'?0:2,truncated:mode==='truncated',recording:{minimumChangeSeconds:10,heartbeatSeconds:1800}};
 }
 else if(u.pathname==='/api/history/status')data={storage:'microSD',available:mode!=='storageError',timeReady:true,successfulWrites:100,writeErrors:mode==='storageError'?1:0,error:mode==='storageError'?'Card read failed':'',clock:{valid:true,source:'fixture',jumps:0},tiers:[]};
 else if(u.pathname==='/api/sd/status')data={mounted:mode!=='storageError',capacityMiB:29808,usedMiB:100,error:mode==='storageError'?'Card read failed':''};
 else if(u.pathname==='/api/monitor/status')data={enabled:true,error:'',clock:{valid:true},modules:[{id:0,connects:1,failures:0,historyWrites:20,dtcSupported:true,dtcBaseline:false},{id:2,connects:1,failures:0,historyWrites:10,dtcSupported:true,dtcBaseline:false}]};
 else if(u.pathname==='/api/modules')data={modules};
 else if(u.pathname==='/api/module/groups')data={groups:[]};
 else if(u.pathname==='/api/dtc'){res.writeHead(200,{'Content-Type':'text/plain'});res.end('Not scanned');return;}
 else if(u.pathname==='/api/log'){res.writeHead(200,{'Content-Type':'text/plain'});res.end('Fixture');return;}
 res.writeHead(code,{'Content-Type':'application/json'});res.end(JSON.stringify(data));
});
let base;
async function screenshot(page,name){if(process.env.T1N_SCREENSHOT_DIR){fs.mkdirSync(process.env.T1N_SCREENSHOT_DIR,{recursive:true});await page.screenshot({path:path.join(process.env.T1N_SCREENSHOT_DIR,name+'.png')});}}
async function piTests(browser,name){
 const page=await browser.newPage({viewport:{width:800,height:480},hasTouch:true}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 mode='normal';moduleId=0;connected=1;writes=[];
 try{
  await page.goto(base+'/pi');await page.waitForFunction(()=>document.querySelector('#gauges').textContent.includes('812'));
  await page.waitForFunction(()=>document.querySelector('#states').textContent.includes('SD history: available'));
  await screenshot(page,name+'-pi-dashboard');
  await page.click('[data-page="trends"]');await page.waitForFunction(()=>document.querySelector('#chart svg'));
  assert.equal(await page.evaluate(()=>state.series.length),2);
  assert.deepEqual(await page.evaluate(()=>state.series[0].points),points);
  assert.match(await page.evaluate(()=>T1NData.exportCsv(state.series)),/utc/i);
  await screenshot(page,name+'-pi-trends');
  await page.reload();await page.click('[data-page="trends"]');await page.waitForFunction(()=>document.querySelector('#chart svg'));
  assert.deepEqual(await page.evaluate(()=>state.series[0].points),points,'Pi reload must re-read the same persisted records');
  // The diagnostic session can disappear or visit transmission without changing the chart selection.
  connected=0;await page.evaluate(()=>poll());await page.click('#refreshHistory');await page.waitForFunction(()=>document.querySelector('#chart svg'));
  assert.match(await page.locator('#states').textContent(),/ECU: disconnected/);
  assert.equal(await page.evaluate(()=>state.historyModule),0);
  moduleId=2;await page.evaluate(()=>poll());assert.equal(await page.evaluate(()=>state.historyModule),0);
  assert.doesNotMatch(await page.locator('#gauges').textContent(),/812/);
  // Shared client handles bounded loading responses, then returns source records.
  mode='pending';pending=0;await page.selectOption('#historyRange','30d');await page.waitForFunction(()=>document.querySelector('#chart svg'));
  mode='historyError';await page.selectOption('#historyRange','7d');await page.waitForFunction(()=>document.querySelector('#historyMessage').textContent.includes('Card read failed'));
  assert.equal(await page.locator('#chart svg').count(),0);assert.equal(await page.locator('#exportCsv').isDisabled(),true);
  mode='empty';await page.selectOption('#historyRange','1h');await page.waitForFunction(()=>document.querySelector('#historyMessage').textContent.includes('No saved records'));
  mode='truncated';await page.selectOption('#historyRange','all');await page.waitForFunction(()=>document.querySelector('#historyMessage').textContent.includes('response limited'));
  // SD failure is independent of valid live readings.
  mode='storageError';moduleId=0;connected=1;await page.evaluate(async()=>{await refreshHealth();await poll();});
  assert.match(await page.locator('#gauges').textContent(),/812/);assert.match(await page.locator('#states').textContent(),/SD history: unavailable/);
  // A sensor API error keeps scanner online, clears readings and explains the sensor error.
  mode='sensorError';await page.evaluate(()=>poll());assert.match(await page.locator('#scanner').textContent(),/Scanner online/);assert.doesNotMatch(await page.locator('#gauges').textContent(),/812/);
  mode='stale';await page.evaluate(()=>poll());assert.doesNotMatch(await page.locator('#gauges').textContent(),/812/);
  mode='offline';await page.evaluate(()=>poll());assert.match(await page.locator('#scanner').textContent(),/Scanner unavailable/);
  mode='normal';await page.evaluate(()=>poll());assert.match(await page.locator('#scanner').textContent(),/Scanner online/);assert.match(await page.locator('#gauges').textContent(),/812/);
  for(const section of ['dashboard','alerts','modules','system'])await page.click('[data-page="'+section+'"]');
  await page.click('[data-page="trends"]');await page.selectOption('#historyModule','2');await page.waitForFunction(()=>document.querySelector('#chart svg'));assert.equal(await page.evaluate(()=>state.snapshot.status.moduleId),0);assert.equal(await page.evaluate(()=>state.historyModule),2);
  assert.equal(writes.length,0,'Pi navigation and reconnect must never send diagnostic or clock POSTs');
  assert.deepEqual(errors,[],'Pi page exceptions');
 }finally{await page.close();}
}
async function mobileTests(browser,name){
 // Implemented below against the canonical mobile page, not a second fixture UI.
 const page=await browser.newPage({viewport:{width:375,height:812},isMobile:true,hasTouch:true}),errors=[];page.on('pageerror',e=>errors.push(e.message));
 mode='normal';moduleId=0;connected=1;writes=[];
 try{
  await page.goto(base+'/');await page.waitForFunction(()=>document.querySelector('#allCards')?.textContent.includes('coolant'));
  await page.click('[data-v="live"]');await page.locator('#allCards [data-metric-id="coolant"]').click();
  await page.waitForFunction(()=>document.querySelector('#trendPlot svg'));
  await screenshot(page,name+'-mobile-trends');
  await page.evaluate(()=>checkHistoryStorage());assert.match(await page.locator('#historyStorage').textContent(),/history ready/);
  assert.deepEqual(await page.evaluate(()=>multiData()[0].points),points);
  assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'Mobile layout overflows at 375 pixels');
  await page.reload();await page.waitForFunction(()=>document.querySelector('#allCards')?.textContent.includes('coolant'));
  await page.click('[data-v="live"]');await page.locator('#allCards [data-metric-id="coolant"]').click();await page.waitForFunction(()=>document.querySelector('#trendPlot svg'));
  assert.deepEqual(await page.evaluate(()=>multiData()[0].points),points,'Mobile reload must re-read persisted records');
  connected=0;await page.evaluate(()=>refresh());await page.evaluate(()=>loadMultiHistory());assert.equal(await page.locator('#trendPlot svg').count(),1);
  moduleId=2;await page.evaluate(()=>refresh());assert.equal(await page.evaluate(()=>chartModule),0);
  mode='historyError';await page.evaluate(()=>setTrendRange('7d'));await page.waitForFunction(()=>document.querySelector('#trendFoot').textContent.includes('Card read failed'));assert.equal(await page.locator('#trendPlot svg').count(),0);
  mode='empty';await page.evaluate(()=>setTrendRange('1h'));await page.waitForFunction(()=>document.querySelector('#trendFoot').textContent.includes('0 saved'));
  mode='storageError';moduleId=0;connected=1;await page.evaluate(async()=>{await refresh();await checkHistoryStorage();});assert.match(await page.locator('#allCards').textContent(),/812/);assert.match(await page.locator('#historyStorage').textContent(),/Card read failed/);
  mode='sensorError';await page.evaluate(()=>refresh());assert.match(await page.locator('#linkState').textContent(),/Scanner online/);assert.doesNotMatch(await page.locator('#allCards').textContent(),/812/);
  mode='normal';moduleId=0;connected=1;await page.evaluate(()=>refresh());
  await page.evaluate(async()=>{selectModule(2);await historyLoadPromise;});assert.equal(await page.evaluate(()=>chartModule),2);assert.equal(await page.evaluate(()=>S.moduleId),0);
  mode='offline';await page.evaluate(()=>refresh());assert.match(await page.locator('#linkState').textContent(),/offline|unavailable/i);
  mode='normal';await page.evaluate(()=>refresh());
  assert.equal(writes.length,0,'Mobile navigation and reconnect must never send diagnostic or clock POSTs');assert.deepEqual(errors,[],'Mobile page exceptions');
 }finally{await page.close();}
}
(async()=>{
 await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));base='http://127.0.0.1:'+server.address().port;
 try{for(const [engine,name] of [[chromium,'Chromium'],[webkit,'WebKit']]){const browser=await engine.launch({headless:true});try{if(!process.env.T1N_PI_ONLY)await mobileTests(browser,name);await piTests(browser,name);console.log(name+': SD history reload/errors/empty/limited, session independence, stale/offline/recovery, read-only navigation passed');}finally{await browser.close();}}}finally{server.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
