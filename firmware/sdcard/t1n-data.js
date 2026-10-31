/* Shared display model. Measurements are never collected or persisted here.
 * History comes only from /api/history/data; failures never return cached rows. */
(function (root, factory) {
  const api = factory();
  if (typeof module === 'object' && module.exports) module.exports = api;
  else root.T1NData = api;
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  'use strict';
  const MAX_POINTS = 200, MAX_SERIES = 8, FRESH_MS = 24000;
  const pairSpecs = Object.freeze({
    rpm: {target:'idleTarget',title:'Engine speed'},
    lowFuel: {target:'lowFuelMin',title:'Low-side fuel pressure',mode:'min'},
    boost: {target:'boostTarget',title:'Boost / MAP'},
    rail: {target:'railTarget',title:'Rail pressure'},
    gear: {target:'targetGear',title:'Gear',mode:'gear'},
    converterSlip: {target:'converterTargetSlip',title:'Converter slip'},
    shiftCurrentActual: {target:'shiftCurrentTarget',title:'Shift regulator current'},
    modCurrentActual: {target:'modCurrentTarget',title:'Modulating regulator current'}
  });
  const metricCatalog = Object.freeze({
    0: [ ['rpm','Engine RPM','rpm'], ['idleTarget','Idle-speed target','rpm'],
      ['speed','Vehicle speed','km/h'], ['coolant','Coolant temperature','°C'],
      ['oilTemp','Engine oil temperature','°C'], ['battery','Battery voltage','V'],
      ['rail','Rail pressure actual','bar'], ['railTarget','Rail pressure target','bar'],
      ['boost','Boost / MAP actual','hPa'], ['boostTarget','Boost target','hPa'],
      ['load','Calculated engine load','%'], ['lowFuel','Low-side fuel pressure','bar'],
      ['lowFuelMin','Low-side pressure ECU minimum','bar'] ],
    2: [ ['temp','Transmission temperature','°C'], ['gear','Actual gear',''],
      ['targetGear','Target gear',''], ['converterSlip','Converter slip actual','rpm'],
      ['converterTargetSlip','Converter slip target','rpm'], ['battery','TCM battery voltage','V'],
      ['speed','Rear-wheel vehicle speed','km/h'], ['engineRpm','Engine RPM received by EGS','rpm'],
      ['outputRpm','Output shaft RPM','rpm'], ['turbineRpm','Turbine RPM','rpm'] ]
  });
  Object.keys(metricCatalog).forEach(module => {
    metricCatalog[module].forEach((row,i) => {
      metricCatalog[module][i] = Object.freeze({module:Number(module),id:row[0],label:row[1],unit:row[2]});
    });
    Object.freeze(metricCatalog[module]);
  });
  function failure(code, message) { return Object.assign(new Error(message), {code}); }
  function yes(value) { return value === true || value === 1; }
  function normalizeSensor(item) {
    if (!item || typeof item.id !== 'string') throw failure('INVALID_SENSORS','Invalid sensor record');
    const age = Number(item.ageMs), sample = Number(item.sampleMs);
    const timingValid = Number.isFinite(age) && age >= 0 && Number.isFinite(sample) && sample >= 0;
    return Object.assign({}, item, {valid:yes(item.valid),stale:yes(item.stale) || !timingValid,
      ageMs:timingValid ? age : Infinity,sampleMs:timingValid ? sample : null});
  }
  function sensorFresh(item, receivedMs, nowMs) {
    if (!item || !yes(item.valid) || yes(item.stale)) return false;
    const age = Number(item.ageMs), sample = Number(item.sampleMs);
    const received = Number(receivedMs), now = Number(nowMs === undefined ? Date.now() : nowMs);
    return Number.isFinite(age) && age >= 0 && Number.isFinite(sample) && sample >= 0 &&
      Number.isFinite(received) && Number.isFinite(now) && now >= received && age + now - received <= FRESH_MS;
  }
  function pairDelta(actual, target, options) {
    const opts = options || {}, now = opts.nowMs === undefined ? Date.now() : opts.nowMs;
    const received = opts.receivedMs === undefined ? now : opts.receivedMs;
    const invalid = reason => ({valid:false,value:null,unit:actual && actual.unit || '',reason});
    if (!sensorFresh(actual,received,now) || !sensorFresh(target,received,now)) return invalid('Measurements unavailable or stale');
    if (Math.abs(actual.sampleMs-target.sampleMs) > (opts.maxSkewMs === undefined ? 2000 : opts.maxSkewMs))
      return invalid('Measurements are not time-aligned');
    if (actual.unit !== target.unit) return invalid('Measurement units differ');
    if (String(actual.value).trim() === '' || String(target.value).trim() === '') return invalid('Comparison is not numeric');
    const a = Number(actual.value), b = Number(target.value);
    if (!Number.isFinite(a) || !Number.isFinite(b)) return invalid('Comparison is not numeric');
    return {valid:true,value:a-b,unit:actual.unit,reason:''};
  }
  function validatePoints(points) {
    if (!Array.isArray(points) || points.length > MAX_POINTS) throw failure('INVALID_HISTORY','History points exceed the response limit');
    const rows = points.map(p => {
      if (!p || !Number.isFinite(p.t) || p.t <= 0 || p.t > 4294967295 || !Number.isFinite(p.min) || !Number.isFinite(p.avg) ||
        !Number.isFinite(p.max) || p.min > p.avg || p.avg > p.max || !Number.isInteger(p.count) || p.count < 1 ||
        ![0,1,2].includes(p.tier)) throw failure('INVALID_HISTORY','Malformed SD history record');
      return Object.assign({},p);
    });
    return rows.sort((a,b) => a.t-b.t);
  }
  function validateHistory(data, query) {
    if (!data || data.ok !== true) throw failure(data && data.code || 'HISTORY_UNAVAILABLE',data && data.error || 'SD history unavailable');
    if (data.schemaVersion !== 1 || data.storage !== 'microSD' || Number(data.module) !== query.module ||
      data.id !== query.id || data.range !== query.range || !data.recording ||
      data.recording.minimumChangeSeconds !== 10 || data.recording.heartbeatSeconds !== 1800)
      throw failure('INVALID_HISTORY','Unsupported SD history response');
    const points = validatePoints(data.points);
    if (data.count !== points.length || typeof data.truncated !== 'boolean') throw failure('INVALID_HISTORY','History count mismatch');
    return Object.assign({},data,{points});
  }
  function clone(value) { return JSON.parse(JSON.stringify(value)); }
  function createClient(options) {
    const opts = options || {}, fetcher = opts.fetchImpl || globalThis.fetch.bind(globalThis);
    const now = opts.now || Date.now, base = (opts.baseUrl || '').replace(/\/$/,'');
    const cache = new Map(), pending = new Map();
    let queue = Promise.resolve(), historyQueue = Promise.resolve(), cacheGeneration = 0;
    function serialize(task) {
      const result = queue.then(task);
      queue = result.catch(() => {});
      return result;
    }
    async function getRaw(path) {
      if (typeof path !== 'string' || !/^\/api\/[a-z0-9/_?=&%.+-]+$/i.test(path)) throw failure('INVALID_PATH','Expected an API GET path');
      const controller = typeof AbortController !== 'undefined' ? new AbortController() : null;
      const timeout = controller ? setTimeout(() => controller.abort(),12000) : null;
      try {
        const response = await fetcher(base+path,{method:'GET',cache:'no-store',signal:controller ? controller.signal : undefined});
        let data;
        try { data = await response.json(); } catch (err) { throw failure('INVALID_JSON','Scanner returned malformed JSON'); }
        if (!data || typeof data !== 'object' || Array.isArray(data)) throw failure('INVALID_JSON','Scanner returned an invalid object');
        return {response,data};
      } finally { if (timeout) clearTimeout(timeout); }
    }
    const get = path => serialize(() => getRaw(path));
    async function request(path) {
        const {response,data} = await get(path);
        if (!response.ok) throw failure(data.code || 'HTTP_'+response.status,data.error || 'Scanner request failed ('+response.status+')');
        return data;
    }
    async function snapshot() {
      const status = await request('/api/status');
      if (!Number.isInteger(status.moduleId)) throw failure('INVALID_STATUS','Invalid scanner status');
      try {
        const sensors = await request('/api/sensors');
        if (typeof sensors.module !== 'string' || !Array.isArray(sensors.items) || sensors.items.length > 256)
          throw failure('INVALID_SENSORS','Invalid scanner sensor response');
        if (status.module && status.module !== sensors.module)
          throw failure('MODULE_CHANGED','Diagnostic module changed during refresh');
        return {online:true,receivedMs:now(),status,sensors:{module:sensors.module,moduleId:status.moduleId,items:sensors.items.map(normalizeSensor)}};
      } catch (err) {
        return {online:true,receivedMs:now(),status,sensors:{module:status.module,moduleId:status.moduleId,items:[]},
          sensorsError:err.message || 'Sensor readings unavailable',sensorsErrorCode:err.code || 'SENSORS_UNAVAILABLE'};
      }
    }
    function history(query) {
      const q = {module:Number(query.module),id:String(query.id),range:String(query.range || '24h')};
      if (!Number.isInteger(q.module) || q.module < 0 || q.module > 5 || !/^[a-z0-9_]+$/i.test(q.id) || !['1h','24h','7d','30d','all'].includes(q.range))
        return Promise.reject(failure('INVALID_QUERY','Invalid history selection'));
      const key = q.module+':'+q.id+':'+q.range, saved = cache.get(key);
      if (!query.force && saved && now()-saved.savedMs >= 0 && now()-saved.savedMs < 15000) return Promise.resolve(clone(saved.data));
      if (pending.has(key)) return pending.get(key).then(clone);
      const generation = cacheGeneration;
      const result = historyQueue.then(async () => {
        const started = Date.now();
        try {
          for (let attempt=0; attempt<120 && Date.now()-started<30000; attempt++) {
            const {response,data} = await get('/api/history/data?module='+q.module+'&id='+encodeURIComponent(q.id)+'&range='+q.range);
            if ((response.status === 202 && data.code === 'HISTORY_LOADING') || (response.status === 503 && data.code === 'HISTORY_QUERY_BUSY')) {
              const delay = Math.max(50,Math.min(1000,Number(data.retryAfterMs) || 250));
              await new Promise(resolve => setTimeout(resolve,delay));
              continue;
            }
            if (!response.ok) throw failure(data.code || 'HTTP_'+response.status,data.error || 'SD history request failed');
            const validated = validateHistory(data,q);
            cache.delete(key);
            if (generation === cacheGeneration) cache.set(key,{savedMs:now(),data:validated});
            while (cache.size>16) cache.delete(cache.keys().next().value);
            return validated;
          }
          throw failure('HISTORY_TIMEOUT','SD history query did not finish; please retry');
        } catch (err) { cache.clear(); cacheGeneration++; throw err; }
      });
      historyQueue = result.catch(() => {});
      pending.set(key,result);
      result.then(() => pending.delete(key),() => pending.delete(key));
      return result.then(clone);
    }
    return {request,snapshot,history,historyStatus:() => request('/api/history/status'),invalidateHistory:() => { cache.clear(); cacheGeneration++; }};
  }
  const escape = value => String(value === undefined ? '' : value).replace(/[&<>"']/g,c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const palette = ['#4abfff','#f7ba52','#66dca9','#ed829e','#ae94ff','#e0dc72','#8dd9db','#f29261'];
  function prepareSeries(series) {
    if (!Array.isArray(series) || series.length > MAX_SERIES) throw failure('INVALID_SERIES','Select up to eight history series');
    return series.map((s,i) => Object.assign({},s,{unit:s.unit || '',points:validatePoints(s.points),
      color:/^#[0-9a-f]{3}([0-9a-f]{3})?$/i.test(s.color || '') ? s.color : palette[i]}));
  }
  function splitGaps(points) {
    const segments = [];
    points.forEach(p => {
      const segment = segments[segments.length-1], prev = segment && segment[segment.length-1];
      const allowed = tier => tier === 2 ? 129600 : tier === 1 ? 1350 : 1830;
      if (!prev || p.t-prev.t > Math.max(allowed(p.tier),allowed(prev.tier))) segments.push([p]);
      else segment.push(p);
    });
    return segments;
  }
  function renderHistorySvg(series, options) {
    const all = prepareSeries(series), opts = options || {};
    const width = Math.max(240,Math.min(1600,Number(opts.width) || 800));
    const panelHeight = Math.max(180,Math.min(500,Number(opts.height) || 260));
    const units = Array.from(new Set(all.map(s => s.unit))), panelCount = Math.max(1,units.length);
    const points = all.flatMap(s => s.points), height = panelHeight*panelCount;
    let svg = '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 '+width+' '+height+'" role="img" aria-label="Saved microSD measurements; gaps indicate missing records" style="width:100%;height:auto;background:#101b28;color:#b7c4d3;font:12px system-ui">';
    if (!points.length) return svg+'<text x="20" y="40" fill="currentColor">No saved measurements in this range</text></svg>';
    let tMin = Math.min(...points.map(p => p.t)), tMax = Math.max(...points.map(p => p.t));
    if (tMax === tMin) { tMin-=1; tMax+=1; }
    units.forEach((unit,panel) => {
      const group = all.filter(s => s.unit===unit), rows = group.flatMap(s => s.points), top=panel*panelHeight;
      const left=70,right=width-18,yTop=top+45,yBottom=top+panelHeight-38;
      let lo=rows.length ? Math.min(...rows.map(p => p.min)) : 0, hi=rows.length ? Math.max(...rows.map(p => p.max)) : 1;
      if (lo === hi) { const pad=Math.max(1,Math.abs(lo)*.05); lo-=pad; hi+=pad; }
      const x=t => left+(t-tMin)/(tMax-tMin)*(right-left), y=v => yBottom-(v-lo)/(hi-lo)*(yBottom-yTop);
      const xy=(p,field) => x(p.t).toFixed(2)+','+y(p[field]).toFixed(2);
      svg+='<g data-unit="'+escape(unit)+'"><text x="12" y="'+(top+18)+'" fill="currentColor">'+escape(unit || 'Value')+'</text>';
      [0,.5,1].forEach(f => {
        const yy=yBottom-f*(yBottom-yTop), val=lo+f*(hi-lo);
        svg+='<path d="M'+left+' '+yy+'H'+right+'" stroke="#344252" stroke-width="1"/><text x="'+(left-7)+'" y="'+(yy+4)+'" text-anchor="end" fill="currentColor">'+escape(Number(val.toPrecision(5)))+'</text>';
      });
      group.forEach((s,i) => {
        svg+='<text x="'+(left+(i%3)*(right-left)/3)+'" y="'+(top+16+Math.floor(i/3)*13)+'" fill="'+s.color+'">'+escape(s.label || s.id)+'</text>';
        splitGaps(s.points).forEach(segment => {
          if (segment.length>1) svg+='<polygon class="history-envelope" points="'+segment.map(p => xy(p,'max')).concat(segment.slice().reverse().map(p => xy(p,'min'))).join(' ')+'" fill="'+s.color+'" opacity="0.16"/>';
          svg+='<path class="history-average" d="'+segment.map((p,j) => (j?'L':'M')+xy(p,'avg')).join(' ')+'" stroke="'+s.color+'" stroke-width="2" fill="none"/>';
          segment.forEach(p => {
            if (p.min!==p.max) svg+='<path class="history-range" d="M'+xy(p,'min')+'L'+xy(p,'max')+'" stroke="'+s.color+'" opacity="0.5"/>';
            svg+='<circle cx="'+x(p.t).toFixed(2)+'" cy="'+y(p.avg).toFixed(2)+'" r="2.5" fill="'+s.color+'"><title>'+escape(new Date(p.t*1000).toISOString()+' · '+(s.label || s.id)+' · avg '+p.avg+' '+unit+' · min '+p.min+' · max '+p.max+' · '+p.count+' contributing samples')+'</title></circle>';
          });
        });
      });
      [tMin,tMax].forEach((t,i) => { svg+='<text x="'+(i?right:left)+'" y="'+(yBottom+23)+'" text-anchor="'+(i?'end':'start')+'" fill="currentColor">'+escape(new Date(t*1000).toISOString().slice(0,19).replace('T',' ')+' UTC')+'</text>'; });
      svg+='</g>';
    });
    return svg+'</svg>';
  }
  function exportCsv(series) {
    const quote = value => '"'+String(value === undefined ? '' : value).replace(/"/g,'""')+'"';
    const lines = ['utc,module,sensor_id,sensor_label,units,average,minimum,maximum,sample_count,tier'];
    prepareSeries(series).forEach(s => s.points.forEach(p => lines.push([
      new Date(p.t*1000).toISOString(),s.module,s.id,s.label,s.unit,p.avg,p.min,p.max,p.count,
      ['recent','15m','daily'][p.tier]
    ].map(quote).join(','))));
    return lines.join('\r\n')+'\r\n';
  }
  return {createClient,normalizeSensor,sensorFresh,pairDelta,pairSpecs,metricCatalog,renderHistorySvg,exportCsv};
});
