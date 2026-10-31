'use strict';
const assert = require('node:assert/strict');
const D = require('../firmware/sdcard/t1n-data.js');
const row = (t=1770000000, avg=10, tier=0) => ({t,min:avg-2,max:avg+2,avg,count:3,tier});
const history = (query={},points=[row()]) => ({ok:true,schemaVersion:1,storage:'microSD',module:0,id:'rpm',range:'24h',
  points,count:points.length,truncated:false,recording:{minimumChangeSeconds:10,heartbeatSeconds:1800},...query});
const response = (data,status=200) => ({ok:status>=200 && status<300,status,json:async () => structuredClone(data)});
const live = {id:'rpm',label:'Engine RPM',value:'700',unit:'rpm',valid:1,stale:0,sampleMs:1000,ageMs:10};
const tests = [];
function test(name,fn) { tests.push({name,fn}); }
function q(id='rpm') { return {module:0,id,range:'24h'}; }
function makeSeries(points,unit='rpm') { return {module:0,id:'rpm',label:'Engine RPM',unit,points}; }

test('Reloads obtain identical SD rows, never synthesize samples',async () => {
  let calls=0;
  const fetchImpl=async () => { calls++; return response(history()); };
  const first=await D.createClient({fetchImpl}).history(q());
  const reload=await D.createClient({fetchImpl}).history(q());
  assert.deepEqual(first,reload); assert.equal(calls,2);
  assert.equal(first.points.length,1); assert.equal(first.points[0].t,1770000000);
});
test('Only valid server rows cached, expired/error cache never falls back',async () => {
  let now=0, mode='good', calls=0;
  const client=D.createClient({now:()=>now,fetchImpl:async () => {
    calls++; if(mode==='offline') throw Error('Network unavailable');
    if(mode==='failed') return response({ok:false,code:'SD_UNAVAILABLE',error:'SD unavailable',points:[]},503);
    if(mode==='invalid') return response(history({},[row(1770000000,NaN)]));
    return response(history());
  }});
  const a=await client.history(q()); a.points[0].avg=999;
  assert.equal((await client.history(q())).points[0].avg,10); assert.equal(calls,1);
  mode='failed'; await assert.rejects(client.history({...q(),force:true}),{code:'SD_UNAVAILABLE'});
  mode='offline'; await assert.rejects(client.history(q()),/Network unavailable/);
  mode='good'; await client.history(q()); now=15001; mode='invalid';
  await assert.rejects(client.history(q()),{code:'INVALID_HISTORY'});
});
test('Cache bounds, explicit invalidation, and invalidation during flight',async () => {
  let calls=0, release;
  const client=D.createClient({now:()=>0,fetchImpl:async path => {
    calls++; const id=new URL(path,'http://scanner').searchParams.get('id');
    if(id==='pending') await new Promise(resolve=>{release=resolve;});
    return response(history({id}));
  }});
  for(let i=0;i<17;i++) await client.history(q('metric'+i));
  await client.history(q('metric0')); assert.equal(calls,18);
  client.invalidateHistory(); await client.history(q('metric0')); assert.equal(calls,19);
  const pending=client.history(q('pending'));
  while(!release) await new Promise(resolve=>setImmediate(resolve));
  client.invalidateHistory(); release(); await pending;
  // A result begun before invalidation must not refill the cache.
  const again=client.history(q('pending'));
  await new Promise(resolve=>setImmediate(resolve)); release(); await again;
  assert.equal(calls,21);
});
test('HTTP requests serialized, identical histories coalesced',async () => {
  let active=0,maximum=0,calls=0;
  const client=D.createClient({fetchImpl:async path => {
    calls++; maximum=Math.max(maximum,++active);
    await new Promise(resolve=>setTimeout(resolve,3)); active--;
    const id=new URL(path,'http://scanner').searchParams.get('id');
    return response(history({id}));
  }});
  await Promise.all([client.history(q()),client.history(q()),client.history(q('rail')),client.request('/api/history/status')]);
  assert.equal(maximum,1); assert.equal(calls,3);
});
test('Cooperative query loading permits live status during retry wait',async () => {
  const paths=[]; let attempts=0;
  const client=D.createClient({fetchImpl:async path => {
    paths.push(path);
    if(path.startsWith('/api/history/data')) return ++attempts===1 ? response({ok:false,code:'HISTORY_LOADING',retryAfterMs:50,points:[]},202) : response(history());
    if(path==='/api/status') return response({moduleId:0,module:'Engine',connected:0});
    return response({module:'Engine',items:[live]});
  }});
  const loading=client.history(q());
  await new Promise(resolve=>setTimeout(resolve,5));
  const snapshot=await client.snapshot(); assert.equal(snapshot.online,true); assert.equal(snapshot.status.connected,0);
  await loading;
  assert.deepEqual(paths.map(p=>p.split('?')[0]),['/api/history/data','/api/status','/api/sensors','/api/history/data']);
});
test('Busy and loading retries bounded; permanent loading reports timeout',async () => {
  let attempts=0;
  const busy=D.createClient({fetchImpl:async () => ++attempts===1 ? response({ok:false,code:'HISTORY_QUERY_BUSY',retryAfterMs:50},503) : response(history())});
  await busy.history(q()); assert.equal(attempts,2);
  // Only advance query-retry timers; keep abort timers real for meaningful request behavior.
  const original=global.setTimeout;
  global.setTimeout=(fn,ms,...args)=>ms<=1000 ? original(fn,0,...args) : original(fn,ms,...args);
  try {
    attempts=0;
    const loading=D.createClient({fetchImpl:async () => { attempts++;return response({ok:false,code:'HISTORY_LOADING',retryAfterMs:50},202); }});
    await assert.rejects(loading.history(q()),{code:'HISTORY_TIMEOUT'}); assert.equal(attempts,120);
  } finally { global.setTimeout=original; }
});
test('Malformed, mismatched and excessive responses rejected',async () => {
  for(const bad of [history({storage:'SPIFFS'}),history({schemaVersion:0}),history({id:'rail'}),history({count:2}),history({points:[row(0)]}),
    history({points:[row(1770000000,10,3)]}),history({},Array.from({length:201},()=>row()))]) {
    await assert.rejects(D.createClient({fetchImpl:async()=>response(bad)}).history(q()),{code:'INVALID_HISTORY'});
  }
  const client=D.createClient({fetchImpl:async()=>({ok:true,status:200,json:async()=>{throw Error('bad');}})});
  await assert.rejects(client.history(q()),{code:'INVALID_JSON'});
  await assert.rejects(client.history({module:0,id:'rpm',range:'live'}),{code:'INVALID_QUERY'});
  await assert.rejects(client.request('http://wrong/api/status'),{code:'INVALID_PATH'});
});
test('Scanner availability, ECU disconnected, sensor failure remain independent',async () => {
  let broken=false;
  const client=D.createClient({now:()=>2000,fetchImpl:async path=>path==='/api/status' ? response({moduleId:0,module:'Engine',connected:0,error:'ECU timeout'}) :
    broken ? response({error:'Sensor memory exhausted'},503) : response({module:'Engine',items:[live]})});
  const snap=await client.snapshot(); assert.equal(snap.online,true); assert.equal(snap.status.connected,0);
  assert.equal(snap.receivedMs,2000); assert.equal(snap.sensors.items[0].valid,true);
  broken=true; const failed=await client.snapshot(); assert.equal(failed.online,true); assert.equal(failed.sensors.items.length,0); assert.match(failed.sensorsError,/Sensor memory exhausted/);
  await assert.rejects(D.createClient({fetchImpl:async()=>{throw Error('Scanner unreachable');}}).snapshot(),/Scanner unreachable/);
});
test('Freshness advances between polls; missing timing is never fresh',() => {
  const item=D.normalizeSensor(live);
  assert.equal(D.sensorFresh(item,10000,20000),true);
  assert.equal(D.sensorFresh(item,10000,35000),false);
  assert.equal(D.sensorFresh({...item,stale:true},10000,10000),false);
  assert.equal(D.sensorFresh(D.normalizeSensor({...live,sampleMs:undefined}),10000,10000),false);
  assert.equal(D.sensorFresh(item,10000,9999),false);
});
test('Actual/target comparison requires valid fresh aligned matching units',() => {
  const a=D.normalizeSensor(live),t={...a,value:'680',sampleMs:1200};
  assert.deepEqual(D.pairDelta(a,t,{receivedMs:1000,nowMs:2000}),{valid:true,value:20,unit:'rpm',reason:''});
  assert.equal(D.pairDelta(a,{...t,sampleMs:4001},{receivedMs:1000,nowMs:2000}).valid,false);
  assert.equal(D.pairDelta(a,{...t,unit:'bar'},{receivedMs:1000,nowMs:2000}).valid,false);
  assert.equal(D.pairDelta(a,t,{receivedMs:1000,nowMs:26000}).valid,false);
});
test('Charts preserve min/max, split missing intervals, and separate units',() => {
  const svg=D.renderHistorySvg([makeSeries([row(),row(1770000010),row(1770005000)]),makeSeries([row()], '°C')]);
  assert.equal((svg.match(/class="history-average"/g)||[]).length,3);
  assert.equal((svg.match(/class="history-envelope"/g)||[]).length,1);
  assert.equal((svg.match(/class="history-range"/g)||[]).length,4);
  assert.match(svg,/data-unit="rpm"/); assert.match(svg,/data-unit="°C"/); assert.match(svg,/UTC/);
  const aggregate=D.renderHistorySvg([makeSeries([row(1770000000,10,1),row(1770000900,12,1),row(1770002700,15,1)])]);
  assert.equal((aggregate.match(/class="history-average"/g)||[]).length,2);
  assert.match(D.renderHistorySvg([makeSeries([])]),/No saved measurements/);
  assert.throws(()=>D.renderHistorySvg(Array.from({length:9},()=>makeSeries([]))),{code:'INVALID_SERIES'});
});
test('CSV contains exact server records, metadata and UTC; escapes labels',() => {
  const series={...makeSeries([row(1770000100,8,1),row(1770000000,10,0)]),label:'Rail "actual", pressure'};
  const csv=D.exportCsv([series]);
  assert.match(csv,/utc,module,sensor_id,sensor_label,units,average,minimum,maximum,sample_count,tier/);
  assert.match(csv,/"Rail ""actual"", pressure"/);assert.match(csv,/"10","8","12","3","recent"/);
  assert.match(csv,/"8","6","10","3","15m"/);assert.match(csv,/Z"/);
  assert.equal(csv.split('\r\n').length,4);
});
(async()=>{
  for(const {name,fn} of tests) {await fn(); console.log('PASS '+name);}
  console.log(tests.length+' shared-client tests passed');
})().catch(error=>{console.error(error);process.exitCode=1;});
