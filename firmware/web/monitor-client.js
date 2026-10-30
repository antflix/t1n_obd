/* Shared by mobile, Pi and the embedded recovery page. No measurements persist here. */
(function(root,factory){const api=factory();if(typeof module==='object'&&module.exports)module.exports=api;else root.T1N=api;})(typeof globalThis!=='undefined'?globalThis:this,function(){
  'use strict';
  const ranges=['1h','24h','7d','30d','all'];
  let tail=Promise.resolve();
  function request(path,options={}){
    const work=async()=>{
      const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),options.timeoutMs||7000);
      try{
        const r=await fetch(path,{cache:'no-store',...options,signal:controller.signal});
        const data=await r.json();
        if(!r.ok||data.ok===false||data.error&&Array.isArray(data.points)){
          const e=new Error(data.error||'HTTP '+r.status);e.code=data.code||'HTTP_ERROR';e.status=r.status;throw e;
        }
        return data;
      }catch(e){if(e.name==='AbortError'){const t=new Error('Scanner request timed out');t.code='TIMEOUT';throw t;}throw e;}
      finally{clearTimeout(timer);}
    };
    const result=tail.then(work,work);tail=result.catch(()=>{});return result;
  }
  function fresh(sensor,elapsedMs=0,maxAge=24000){
    return !!sensor&&(sensor.valid===1||sensor.valid===true)&&!sensor.stale
      &&Number.isFinite(Number(sensor.ageMs))&&Number(sensor.ageMs)>=0
      &&Number(sensor.ageMs)+elapsedMs<=maxAge;
  }
  function aligned(a,b,maxSkewMs=3000){
    return fresh(a)&&fresh(b)&&a.unit===b.unit&&Math.abs(Number(a.ageMs)-Number(b.ageMs))<=maxSkewMs;
  }
  function validateHistory(data){
    if(!data||!Array.isArray(data.points))throw Error('Malformed SD history response');
    if(data.points.length>350)throw Error('SD history page exceeds limit');
    for(const p of data.points){
      if(!['t','avg','min','max','count','tier'].every(k=>typeof p[k]==='number'&&Number.isFinite(p[k]))
        ||p.t<1609459200||p.count<1||p.min>p.avg||p.avg>p.max||p.tier<0||p.tier>2)throw Error('Invalid SD history record');
    }
    return data;
  }
  async function history(module,id,range,maxPoints=400){
    if(!ranges.includes(range)||!Number.isInteger(module)||module<0||module>5)throw Error('Invalid history selection');
    const base='/api/history/data?'+new URLSearchParams({module,id,range});
    let url=base,points=[],data;
    do{
      for(let attempt=0;;attempt++){
        try{data=validateHistory(await request(url));break;}
        catch(e){
          if(!['HISTORY_PENDING','HISTORY_QUERY_BUSY','DIAGNOSTIC_BUSY'].includes(e.code)||attempt>=79)throw e;
          await new Promise(resolve=>setTimeout(resolve,500));
        }
      }
      points.push(...data.points);
      if(!data.more||!data.next||points.length>=maxPoints)break;
      url=base+'&'+new URLSearchParams(data.next);
    }while(true);
    return {...data,points:points.sort((a,b)=>a.t-b.t),limited:!!data.more};
  }
  function segments(points){
    const result=[];
    for(const p of points){
      const group=result[result.length-1],prior=group&&group[group.length-1];
      const gap=p.tier===2?129600:p.tier===1?1350:90;
      if(!prior||p.tier!==prior.tier||p.t-prior.t>gap||p.t<prior.t)result.push([p]);else group.push(p);
    }
    return result;
  }
  function csv(series,moduleId,moduleLabel){
    const quote=v=>'"'+String(v==null?'':v).replace(/"/g,'""')+'"';
    const rows=[['Timestamp UTC','Module ID','Module','Sensor ID','Sensor label','Units','Average','Minimum','Maximum','Sample count','Tier']];
    for(const s of series)for(const p of s.points)rows.push([new Date(p.t*1000).toISOString(),moduleId,moduleLabel,s.id,s.label,s.unit,p.avg,p.min,p.max,p.count,['recent','15m','daily'][p.tier]]);
    return '\uFEFF'+rows.map(r=>r.map(quote).join(',')).join('\r\n')+'\r\n';
  }
  function status(scanner,monitor,storage,online){
    return {scannerOnline:!!online,monitoringEnabled:monitor?.enabled===true,ecuConnected:!!online&&!!scanner?.connected,
      historyAvailable:!!storage&&storage.storage==='microSD'&&storage.enabled===true,
      storageHealthy:!!storage&&storage.enabled===true&&storage.storage==='microSD'&&!storage.error,
      clockValid:!!monitor?.clock?.valid,ignitionQualified:monitor?.ignitionQualified===true};
  }
  return {request,fresh,aligned,history,validateHistory,segments,csv,status,ranges};
});
