'use strict';
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const D=require('../firmware/sdcard/t1n-data.js');
const source=fs.readFileSync(require('node:path').join(__dirname,'../firmware/t1n_obd/50_scanner_ui.ino'),'utf8');
const script=source.match(/<script>([\s\S]*?)<\/script>/)[1];
function element(id){
  const classes=new Set(['overview'].includes(id)?['on']:[]);
  return {id,textContent:'',innerHTML:'',value:'',checked:true,style:{},dataset:{},clientWidth:375,
    classList:{add:x=>classes.add(x),remove:x=>classes.delete(x),contains:x=>classes.has(x),toggle(x,state){if(state===undefined)state=!classes.has(x);state?classes.add(x):classes.delete(x);}},
    setAttribute(){},blur(){},focus(){},querySelectorAll:()=>[],scrollIntoView(){},appendChild(){},click(){},remove(){}};
}
function fixture(){
  const nodes=new Map(),requests=[],timers=[],views=['overview','live','faults','tests','modules','logs'];
  const get=id=>nodes.get(id)||(nodes.set(id,element(id)),nodes.get(id));
  let active=0,network=true,historyAvailable=true,sensorsValid=true;
  const fetch=async(path,opts={})=>{
    requests.push({path,method:opts.method||'GET'});
    if(!network)throw Error('Scanner unreachable');
    const status={moduleId:active,module:active===0?'Engine / CR2':'Transmission / EGS',connected:0,polling:0,firmware:'fixture',error:'ECU disconnected'};
    const q=new URL(path,'http://scanner').searchParams;
    let data;
    if(path==='/api/status')data=status;
    else if(path==='/api/sensors'){
      if(!sensorsValid)return {ok:true,status:200,json:async()=>{throw Error('Malformed response');}};
      data={module:status.module,items:[{id:active===0?'rpm':'temp',label:'Live fixture',value:active===0?'720':'30',unit:active===0?'rpm':'°C',valid:1,stale:0,sampleMs:100,ageMs:20}]};
    }else if(path==='/api/modules')data={modules:[{id:0,name:'Engine / CR2',pollable:true},{id:2,name:'Transmission / EGS',pollable:true}]};
    else if(path==='/api/module/groups')data={groups:[]};
    else if(path==='/api/history/status')data={available:true,timeReady:true,storage:'microSD'};
    else if(path.startsWith('/api/history/data')){
      if(!historyAvailable)return {ok:false,status:503,json:async()=>({ok:false,code:'SD_UNAVAILABLE',error:'SD removed',points:[]})};
      data={ok:true,schemaVersion:1,storage:'microSD',module:Number(q.get('module')),id:q.get('id'),range:q.get('range'),points:[{t:1770000000,min:680,avg:700,max:720,count:5,tier:1}],count:1,truncated:false,recording:{minimumChangeSeconds:10,heartbeatSeconds:1800}};
    }else data={};
    return {ok:true,status:200,json:async()=>structuredClone(data),text:async()=>JSON.stringify(data)};
  };
  const context=vm.createContext({console,Date,Math,Map,Set,Number,String,Object,Array,Promise,URLSearchParams,Blob,URL,
    document:{getElementById:get,querySelectorAll:selector=>selector==='.view'?views.map(get):[],addEventListener(){},createElement:()=>element('download'),body:element('body')},
    window:{getSelection:()=>''},navigator:{},alert(){},confirm:()=>false,
    setTimeout(fn,ms){timers.push({fn,ms});return timers.length;},clearTimeout(){},fetch,
    T1NData:{...D,createClient:()=>D.createClient({fetchImpl:fetch})}});
  vm.runInContext(script,context);
  async function settle(){for(let i=0;i<8;i++)await new Promise(resolve=>setImmediate(resolve));}
  return {context,requests,timers,get,settle,run:code=>vm.runInContext(code,context),setActive:n=>{active=n;},setNetwork:n=>{network=n;},setHistory:n=>{historyAvailable=n;},setSensors:n=>{sensorsValid=n;}};
}
(async()=>{
  const f=fixture();await f.settle();
  assert.equal(f.get('linkState').textContent,'Scanner online');assert.match(f.get('pollBadge').textContent,/ECU disconnected/);
  assert.equal(f.requests.some(r=>r.method==='POST'),false);
  await f.run("showView('modules');selectModule(2);historyLoadPromise");
  assert.equal(f.run('S.moduleId'),0);assert.equal(f.run('chartModule'),2);
  assert.equal(f.requests.some(r=>r.path.includes('module/select')),false);
  assert.equal(f.requests.some(r=>r.method==='POST'),false);assert.match(f.get('trendPlot').innerHTML,/<svg/);
  const records=f.run('JSON.stringify(multiData()[0].points)');
  f.setActive(2);await f.run('refresh()');assert.equal(f.run('chartModule'),2);
  f.setActive(0);await f.run('refresh()');assert.equal(f.run('chartModule'),2);assert.equal(f.run('selectedTrend'), 'temp');
  const reload=fixture();await reload.settle();await reload.run("showTrend('temp',2);historyLoadPromise");
  assert.equal(reload.run('JSON.stringify(multiData()[0].points)'),records);
  f.setHistory(false);await f.run('loadMultiHistory(true)');assert.equal(f.get('trendPlot').innerHTML,'');assert.match(f.get('trendFoot').textContent,/History unavailable: SD removed/);
  assert.match(f.get('keyCards').innerHTML,/720/); // History failure leaves live data intact.
  f.setSensors(false);await f.run('refresh()');assert.equal(f.get('linkState').textContent,'Scanner online');assert.equal(f.get('keyCards').innerHTML,'');assert.match(f.get('sensorError').textContent,/malformed JSON/);
  f.setSensors(true);f.setNetwork(false);await f.run('refresh()');assert.equal(f.get('linkState').textContent,'Scanner offline');assert.equal(f.get('allCards').innerHTML,'');
  assert.match(f.get('trendFoot').textContent,/Scanner unreachable/);
  assert.equal(f.requests.some(r=>r.method==='POST'),false);
  assert.ok(f.timers.some(t=>t.ms===3000));assert.equal(f.run('typeof rememberItems'),'undefined');
  assert.equal(f.run('typeof multiBrowserPoints'),'undefined');assert.equal(f.run('typeof history'),'undefined');
  console.log('PASS mobile fixtures: no page/navigation POST, independent module history, reload persistence, storage/sensor/network failures, no browser history');
})().catch(e=>{console.error(e);process.exitCode=1;});
