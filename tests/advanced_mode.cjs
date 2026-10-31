'use strict';
// Execute the actual Advanced page script; opening a page never owns the UART.
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm'),assert=require('node:assert/strict');
const source=fs.readFileSync(path.join(__dirname,'../firmware/t1n_obd/40_advanced_ui.ino'),'utf8');
const match=source.match(/<script>([\s\S]*?)<\/script>/);assert.ok(match,'Production Advanced script exists');
function fixture(options={}){
 const nodes=new Map(),requests=[],timers=[],alerts=[];
 const control={automatic:true,connected:0,polling:0,postStatus:200,postNetwork:false,monitorNetwork:false,statusNetwork:false,malformedMonitor:false,malformedStatus:false,confirmationMismatch:false,postGate:null,...options};
 const get=id=>{if(!nodes.has(id))nodes.set(id,{id,textContent:'',value:'',disabled:false});return nodes.get(id);};
 function response(status,data){return {ok:status>=200&&status<300,status,json:async()=>structuredClone(data),text:async()=>typeof data==='string'?data:JSON.stringify(data)};}
 const fetch=async(url,opts={})=>{
  const method=opts.method||'GET';requests.push({url,method,body:opts.body?String(opts.body):''});
  if(url==='/api/monitor/enable'){
   assert.equal(method,'POST');
   if(control.postGate)await control.postGate;
   if(control.postNetwork)throw Error('Scanner unreachable during mode request');
   if(control.postStatus!==200)return response(control.postStatus,'Diagnostic link busy');
   if(!control.confirmationMismatch)control.automatic=new URLSearchParams(opts.body).get('enabled')==='1';
   return response(200,{enabled:control.automatic});
  }
  assert.equal(method,'GET','Only explicit mode changes may send POST');
  if(url==='/api/monitor/status'){
   if(control.monitorNetwork)throw Error('Scanner unreachable during confirmation');
   return response(200,{enabled:control.malformedMonitor?'yes':control.automatic});
  }
  if(url==='/api/status'){
   if(control.statusNetwork)throw Error('ECU status confirmation unavailable');
   return response(200,{connected:control.malformedStatus?'unknown':control.connected,polling:control.polling});
  }
  if(url==='/config')return response(200,{});
  if(url==='/api/relay/status')return response(200,{channels:[]});
  if(url==='/status')return response(200,{connected:control.connected,polling:control.polling});
  if(url==='/live')return response(200,{text:''});
  if(url==='/api/module/groups')return response(200,{groups:[]});
  if(['/dtc','/log','/api/activity-log'].includes(url))return response(200,'');
  throw Error('Unexpected request '+url);
 };
 const context=vm.createContext({console,Date,JSON,Promise,URLSearchParams,fetch,
  document:{getElementById:get},window:{getSelection:()=>({toString:()=>''})},navigator:{},
  alert:message=>alerts.push(message),confirm:()=>{throw Error('Unexpected diagnostic confirmation');},
  setInterval(fn,ms){timers.push({fn,ms});return timers.length;},clearInterval(){},
  setTimeout(){throw Error('Mode changes must not schedule retries');}});
 vm.runInContext(match[1],context);
 async function settle(){for(let i=0;i<8;i++)await new Promise(resolve=>setImmediate(resolve));}
 const run=code=>vm.runInContext(code,context);
 const posts=()=>requests.filter(r=>r.method==='POST');
 function restored(){for(const id of ['modePause','modeResume','modeRefresh'])assert.equal(get(id).disabled,false,id+' restored');assert.equal(run('modeBusy'),false);}
 function unknown(){assert.match(get('modeState').textContent,/unknown/i);assert.match(get('modeSession').textContent,/unknown/i);assert.notEqual(get('modeError').textContent,'');restored();}
 return {control,get,requests,timers,alerts,settle,run,posts,restored,unknown};
}
(async()=>{
 const f=fixture();await f.settle();
 assert.equal(f.posts().length,0,'Opening Advanced sends only GET requests');
 assert.match(f.get('modeState').textContent,/enabled.*owns the diagnostic connection/);
 assert.match(f.get('modeSession').textContent,/disconnected.*polling off/);
 f.restored();assert.deepEqual(f.alerts,[]);
 for(const enabled of [false,true]){
  const before=f.posts().length;await f.run('setAutomaticMode('+enabled+')');
  assert.equal(f.posts().length,before+1);
  assert.deepEqual(f.posts().at(-1),{url:'/api/monitor/enable',method:'POST',body:'enabled='+(enabled?'1':'0')});
  assert.match(f.get('modeState').textContent,enabled?/enabled/:/paused/);
  assert.match(f.get('modeSession').textContent,/disconnected/);
  assert.equal(f.get('modeError').textContent,'');f.restored();
 }
 // Only one explicit request can run, even if buttons are activated repeatedly.
 let release;f.control.postGate=new Promise(resolve=>{release=resolve;});
 const pending=f.run('setAutomaticMode(false)');
 for(const id of ['modePause','modeResume','modeRefresh'])assert.equal(f.get(id).disabled,true);
 const before=f.posts().length;await f.run('setAutomaticMode(true)');await f.run('refreshOperatingMode()');
 assert.equal(f.posts().length,before);release();await pending;f.control.postGate=null;f.restored();
 // Error paths never retry, connect an ECU, send raw commands or actuate outputs.
 for(const failure of [{postStatus:409},{postNetwork:true},{monitorNetwork:true},{statusNetwork:true},{malformedMonitor:true},{malformedStatus:true},{confirmationMismatch:true}]){
  const broken=fixture();await broken.settle();Object.assign(broken.control,failure);
  const requested=!broken.control.automatic;
  await broken.run('setAutomaticMode('+requested+')');await broken.settle();
  assert.equal(broken.posts().length,1,JSON.stringify(failure)+' sends exactly one POST');
  broken.unknown();assert.deepEqual(broken.alerts,[],'Errors stay inline');
  assert.equal(broken.requests.some(r=>/connect|payload|\/test|dtc\/clear|poll\/start/.test(r.url)),false);
  Object.assign(broken.control,{postStatus:200,postNetwork:false,monitorNetwork:false,statusNetwork:false,malformedMonitor:false,malformedStatus:false,confirmationMismatch:false});
  await broken.run('refreshOperatingMode()');
  assert.equal(broken.posts().length,1,'Explicit status refresh is GET-only');
  assert.equal(broken.get('modeError').textContent,'');broken.restored();
 }
 const unavailable=fixture({monitorNetwork:true});await unavailable.settle();unavailable.unknown();assert.equal(unavailable.posts().length,0);
 assert.equal(f.requests.some(r=>r.method==='POST'&&r.url!=='/api/monitor/enable'),false);
 console.log('PASS Advanced mode fixtures: GET-only opening, independent monitoring/session states, explicit single POST transitions, busy/network/confirmation errors, restored controls and no retries or diagnostic side effects');
})().catch(error=>{console.error(error);process.exitCode=1;});
