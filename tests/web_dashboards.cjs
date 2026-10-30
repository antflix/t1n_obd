// Run with NODE_PATH pointing to a Playwright installation. All APIs are local fixtures.
const assert=require('node:assert/strict'),http=require('node:http'),fs=require('node:fs'),path=require('node:path');
const {chromium,webkit}=require('playwright');
const root=path.join(__dirname,'../firmware/sdcard/www');
let mode='normal',moduleId=0,connected=1,requests=[],writes=[],active=0,maxActive=0;
let pending=0;
const t=1793300000;
const points=[{t:t-200,avg:85,min:82,max:88,count:3,tier:0,serial:1},{t:t-20,avg:90,min:89,max:91,count:2,tier:0,serial:2}];
const sensor=(id,value,unit='°C')=>({id,label:id,value:String(value),unit,valid:1,stale:mode==='stale'?1:0,ageMs:mode==='stale'?40000:100,sampleMs:1000,state:'neutral'});
const server=http.createServer(async(req,res)=>{
 const u=new URL(req.url,'http://localhost');requests.push(u.pathname+u.search);if(req.method!=='GET')writes.push(req.url);
 if(!u.pathname.startsWith('/api/')){const file=u.pathname==='/pi'?'pi.html':'index.html';res.setHeader('Content-Type','text/html');res.end(fs.readFileSync(path.join(root,file)));return;}
 active++;maxActive=Math.max(maxActive,active);res.on('finish',()=>active--);
 if(mode==='offline'){res.destroy();active--;return;}
 let code=200,d={};
 if(u.pathname==='/api/status')d={moduleId,module:moduleId===0?'Engine / CR2':'Transmission / EGS',connected,polling:connected,firmware:'FIXTURE',uptimeMs:2000,status:'fixture'};
 else if(u.pathname==='/api/sensors')d={moduleId,module:'fixture',items:moduleId===2?[sensor('temp',50)]:[sensor('rpm',800,'rpm'),sensor('coolant',85),sensor('battery',14,'V'),sensor('rail',300,'bar'),sensor('railTarget',302,'bar'),sensor('boost',1000,'hPa'),sensor('boostTarget',1002,'hPa')]};
 else if(u.pathname==='/api/monitor/status')d={enabled:true,ignitionQualified:false,clock:{valid:true},modules:[{id:0,connects:1,lastConnectionUptimeMs:100,historyWrites:20},{id:2,connects:1,historyWrites:10}],absAutoQualified:false};
 else if(u.pathname==='/api/history/status'){
  if(mode==='storageStatusError'){code=503;d={error:'Storage health unavailable'};}
  else d={storage:'microSD',enabled:true,timeReady:true,successfulWrites:100,writeErrors:0};
 }
 else if(u.pathname==='/api/history/data'){
  if(mode==='pending'&&pending++<2){code=202;d={ok:false,code:'HISTORY_PENDING',error:'SD query pending',points:[]};}
  else if(mode==='historyError'){code=503;d={ok:false,code:'HISTORY_READ_FAILED',error:'Card read failed',points:[]};}
  else d={ok:true,source:'microSD',points:mode==='empty'?[]:points,count:mode==='empty'?0:2,more:false};
 }
 else if(u.pathname==='/api/alerts')d={rules:[]};
 else if(u.pathname==='/api/modules')d={modules:[]};
 else if(u.pathname==='/api/module/groups')d={groups:[]};
 else if(u.pathname==='/api/monitor/dtc')d={modules:[{id:0,supported:true,baseline:true,codes:[],lastScanEpoch:0}]};
 else if(u.pathname.endsWith('/events'))d={rows:[]};
 else if(u.pathname==='/api/dtc'){res.end('Stored baseline');return;}
 res.writeHead(code,{'Content-Type':'application/json'});res.end(JSON.stringify(d));
});
async function run(engine,name){
 const browser=await engine.launch({headless:true});
 try{
  const page=await browser.newPage({viewport:{width:375,height:812},isMobile:true,hasTouch:true});const errors=[];page.on('pageerror',e=>errors.push(e.message));
  mode='normal';moduleId=0;connected=1;writes=[];
  await page.goto(base+'/');await page.waitForFunction(()=>document.querySelector('#allCards')?.textContent.includes('coolant'));
  await page.click('[data-v="live"]');await page.locator('#allCards [data-metric-id="coolant"]').click();
  await page.waitForFunction(()=>document.querySelectorAll('#trendPlot polyline').length===2);
  if(process.env.T1N_SCREENSHOT_DIR){fs.mkdirSync(process.env.T1N_SCREENSHOT_DIR,{recursive:true});await page.locator('#trend').screenshot({path:path.join(process.env.T1N_SCREENSHOT_DIR,name+'-mobile-trend.png')});}
  assert.deepEqual(await page.evaluate(()=>multiData()[0].points),points.map(p=>({...p,source:'microSD'})));
  assert((await page.evaluate(()=>T1N.csv(multiData(),0,'Engine'))).includes('Sample count'));
  assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth),'375px page overflow');
  // Page reload reads the same records again; no locally collected samples exist.
  await page.reload();await page.waitForFunction(()=>document.querySelector('#allCards')?.textContent.includes('coolant'));
  await page.click('[data-v="live"]');await page.locator('#allCards [data-metric-id="coolant"]').click();
  await page.waitForFunction(()=>document.querySelectorAll('#trendPlot polyline').length===2);
  moduleId=2;await page.evaluate(()=>refresh());assert.equal(await page.evaluate(()=>chartModule),0);assert.equal(await page.locator('#trend').isVisible(),true);
  mode='pending';pending=0;await page.evaluate(()=>setTrendRange('30d'));await page.waitForFunction(()=>document.querySelectorAll('#trendPlot polyline').length===2);
  mode='historyError';await page.evaluate(()=>setTrendRange('24h'));await page.waitForFunction(()=>document.querySelector('#trendFoot').textContent.includes('Card read failed'));
  assert.equal(await page.locator('#trendPlot polyline').count(),0);
  mode='empty';await page.evaluate(()=>setTrendRange('7d'));await page.waitForFunction(()=>document.querySelector('#trendPlot').textContent.includes('No saved SD'));
  await page.click('[data-v="faults"]');await page.evaluate(()=>refresh());await page.waitForFunction(()=>document.querySelector('#dtcSnapshot').textContent.includes('saved code'));
  assert.equal(writes.length,0,'Opening/navigating mobile must issue no POST');assert.deepEqual(errors,[]);
  await page.close();
  const pi=await browser.newPage({viewport:{width:800,height:480},hasTouch:true});const piErrors=[];pi.on('pageerror',e=>piErrors.push(e.message));
  mode='normal';moduleId=0;connected=0;await pi.goto(base+'/pi');await pi.waitForFunction(()=>document.querySelector('#loggerText').textContent==='AUTO STANDBY');
  if(process.env.T1N_SCREENSHOT_DIR)await pi.screenshot({path:path.join(process.env.T1N_SCREENSHOT_DIR,name+'-pi-dashboard.png')});
  await pi.click('[data-nav="trends"]');await pi.waitForFunction(()=>document.querySelector('#trendInfo').textContent.includes('2 saved records'));
  await pi.selectOption('#trendRange','24h');await pi.waitForFunction(()=>document.querySelector('#trendInfo').textContent.includes('24h')&&document.querySelectorAll('#trendChart path').length>0);
  mode='historyError';await pi.selectOption('#trendRange','7d');await pi.waitForFunction(()=>document.querySelector('#trendInfo').textContent.includes('Card read failed'));assert.equal(await pi.locator('#trendChart path').count(),0);
  mode='storageStatusError';await pi.waitForFunction(()=>!state.awaiting);await pi.evaluate(async()=>{state.systemAt=0;await tick()});assert.equal(await pi.evaluate(()=>state.offline),false);
  mode='stale';await pi.evaluate(()=>tick());assert.equal(await pi.evaluate(()=>value(0,'coolant')),NaN);
  mode='offline';await pi.evaluate(()=>tick());assert.equal(await pi.evaluate(()=>state.offline),true);
  mode='normal';await pi.evaluate(()=>tick());assert.equal(await pi.evaluate(()=>state.offline),false);
  await pi.click('[data-nav="system"]');assert((await pi.locator('#systemInfo').textContent()).includes('Unqualified'));
  assert.equal(writes.length,0,'Pi must issue no POST');assert.deepEqual(piErrors,[]);
  await pi.close();console.log(name+': mobile reload/SD-only/error/empty/module rotation/CSV/375px/DTC and Pi offline/reconnect/storage/clock/ranges PASS');
 }finally{await browser.close();}
}
let base;
(async()=>{await new Promise(r=>server.listen(0,'127.0.0.1',r));base='http://127.0.0.1:'+server.address().port;try{await run(chromium,'Chromium');await run(webkit,'WebKit');}finally{server.close();}})().catch(e=>{console.error(e);process.exitCode=1;});
