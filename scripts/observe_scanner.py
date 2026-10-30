#!/usr/bin/env python3
"""Bounded read-only health observation. No session, configuration or reboot writes."""
import argparse,json,time,urllib.request,statistics,pathlib
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--base',default='http://192.168.8.168');ap.add_argument('--samples',type=int,default=6);ap.add_argument('--interval',type=float,default=10);ap.add_argument('--output',required=True,type=pathlib.Path);a=ap.parse_args()
if not 1<=a.samples<=360 or a.interval<2:ap.error('Use 1..360 samples and interval >=2 seconds')
if a.output.exists():ap.error('Output must be new')
latencies=[];errors=0
with a.output.open('x') as out:
 for i in range(a.samples):
  row={'observedUtc':time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime()),'responses':{}}
  for path in ['/api/status','/api/sensors','/api/monitor/status','/api/history/status']:
   t=time.monotonic()
   try:
    with urllib.request.urlopen(a.base.rstrip('/')+path,timeout=4) as r:raw=r.read(131073)
    if len(raw)>131072:raise ValueError('Response exceeds 128 KiB bound')
    d=json.loads(raw)
    if path=='/api/sensors':
     if not isinstance(d.get('items'),list):raise ValueError('Missing sensor catalogue')
     d={'moduleId':d.get('moduleId'),'sensorCount':len(d['items']),'freshCount':sum(bool(x.get('valid')) and not x.get('stale') for x in d['items'])}
    latency=round((time.monotonic()-t)*1000);latencies.append(latency);row['responses'][path]={'bytes':len(raw),'latencyMs':latency,'data':d}
   except (OSError,ValueError) as exc:errors+=1;row['responses'][path]={'error':str(exc),'latencyMs':round((time.monotonic()-t)*1000)}
  out.write(json.dumps(row)+'\n');out.flush()
  if i+1<a.samples:time.sleep(a.interval)
print(json.dumps({'samples':a.samples,'errors':errors,'medianHttpMs':statistics.median(latencies) if latencies else None,'maxHttpMs':max(latencies) if latencies else None}))
