'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const html=fs.readFileSync('firmware/sdcard/pi.html','utf8');
const shared=require('../firmware/sdcard/t1n-data.js');
assert.match(html,/<script src="\/t1n-data\.js"><\/script>/);
assert.doesNotMatch(html,/\bfetch\s*\(|localStorage|indexedDB|sessionStorage|XMLHttpRequest|method\s*:\s*['"]POST|api\/module\/connect|api\/monitor\/clock/);
assert.doesNotMatch(html,/function\s+(?:history|segments|validateHistory|exportCsv|renderHistorySvg)\s*\(/);
for(const range of ['1h','24h','7d','30d','all'])assert.ok(html.includes('value="'+range+'"'));
const nodes=new Map();
function node(id){if(!nodes.has(id))nodes.set(id,{id,innerHTML:'',textContent:'',className:'',disabled:false,hidden:false,value:'',dataset:{},addEventListener(){},setAttribute(){}});return nodes.get(id);}
const pages=['dashboard','trends','alerts','modules','system'].map(node);
let response={points:[{t:1760000000,min:100,max:120,avg:110,count:2,tier:0}],truncated:false};
let historyError=null,historyCalls=[];
const api={history:async query=>{historyCalls.push(query);if(historyError)throw historyError;return response;},request:async()=>({}),historyStatus:async()=>({}),snapshot:async()=>({})};
const context=vm.createContext({T1NData:{...shared,createClient:()=>api},document:{getElementById:node,querySelectorAll:selector=>selector==='.page'?pages:[]},Date,Number,String,Math,Set,Blob,URL,setTimeout(){},setInterval(){}});
const script=html.match(/<script>\s*([\s\S]*?)<\/script>/)[1].replace(/initialize\(\);\s*$/,'');
vm.runInContext(script,context);
const run=source=>vm.runInContext(source,context);
(async()=>{
 // Source records load without a diagnostic session or any prior live sample.
 run("state.online=true;state.snapshot={status:{connected:0},sensors:{moduleId:0,items:[]}};state.receivedMs=Date.now();");
 await run('loadHistory()');
 assert.equal(historyCalls.length,2);
 assert.equal(historyCalls[0].module,0);
 assert.equal(historyCalls[0].range,'24h');
 assert.equal(historyCalls[0].id,'rail');
 assert.ok(node('chart').innerHTML.includes('<svg'));
 assert.equal(run('state.series[0].points[0].t'),1760000000);
 assert.equal(node('exportCsv').disabled,false);
 // A failed subsequent read explicitly removes all previously rendered records.
 historyError=new Error('SD read failed');
 await run('loadHistory()');
 assert.equal(node('chart').innerHTML,'');
 assert.equal(run('state.series.length'),0);
 assert.match(node('historyMessage').textContent,/SD read failed/);
 assert.equal(node('exportCsv').disabled,true);
 // Empty results are distinct from an unavailable backend.
 historyError=null;response={points:[],truncated:false};await run('loadHistory()');
 assert.match(node('historyMessage').textContent,/No saved records/);
 response={points:[{t:1760000000,min:1,max:1,avg:1,count:1,tier:0}],truncated:true};await run('loadHistory()');
 assert.match(node('historyMessage').textContent,/response limited/);
 // A fresh numeric value is hidden on elapsed freshness, and on scanner loss.
 run("state.snapshot={status:{connected:1,firmware:'test'},sensors:{moduleId:0,items:[{id:'rpm',label:'Engine RPM',value:'812',unit:'rpm',valid:true,stale:false,ageMs:10,sampleMs:300}]}};state.online=true;state.receivedMs=Date.now();renderLive();");
 assert.match(node('gauges').innerHTML,/812/);
 run('state.receivedMs=Date.now()-25000;renderLive()');
 assert.doesNotMatch(node('gauges').innerHTML,/812/);
 assert.match(node('gauges').innerHTML,/Stale \/ invalid reading/);
 run('state.receivedMs=Date.now();state.online=false;renderLive()');
 assert.doesNotMatch(node('gauges').innerHTML,/812/);
 assert.match(node('scanner').textContent,/Scanner unavailable/);
 // Health states remain independent: ECU disconnected does not remove SD charts.
 run("state.online=true;state.snapshot.status.connected=0;state.health={history:{available:true,storage:'microSD',clock:{valid:true}},sd:{mounted:true,error:''},monitor:{enabled:true}};renderLive()");
 assert.match(node('states').innerHTML,/ECU: disconnected/);
 assert.match(node('states').innerHTML,/SD history: available/);
 assert.match(node('states').innerHTML,/Monitoring: enabled/);
 assert.match(node('states').innerHTML,/Clock: valid/);
 run("state.health.sd.mounted=false;state.health.history.available=false;renderLive()");
 assert.match(node('states').innerHTML,/SD history: unavailable/);
 assert.match(node('states').innerHTML,/Storage: unavailable/);
 // Viewing a page updates only presentation; history requests remain read-only.
 const count=historyCalls.length;run("navigate('modules')");assert.equal(historyCalls.length,count);
 assert.equal(node('modules').hidden,false);assert.equal(node('dashboard').hidden,true);
 assert.equal(run('state.page'),'modules');
 console.log('Pi view: shared SD history, failure/empty/limited states, stale/offline, independent health, read-only navigation passed');
})().catch(error=>{console.error(error);process.exitCode=1;});
