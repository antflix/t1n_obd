#!/usr/bin/env python3
"""Offline, backup-first history migration candidate builder. Never accesses a device.

Without --current, only validate the trusted recovered CSV. With --current, point
at an offline *backup* directory containing hist_now.dat/hist_15m.dat/hist_day.dat.
Creates a NEW directory with originals, merged candidates, hashes and rollback
manifest. No existing file is replaced; conflicting aggregates stop the merge.
Deployment requires separate parked approval and paused logging.
"""
import argparse,csv,hashlib,json,math,pathlib,struct
RECORD=struct.Struct('<IIIfffII')
TIERS={'recent':('hist_now.dat',32000),'15m':('hist_15m.dat',32000),'daily':('hist_day.dat',16000)}
def checksum(data):
    h=2166136261
    for b in data:h=((h^b)*16777619)&0xffffffff
    return h

def identity(r):return RECORD.pack(*r) [4:28]

def valid(r):
    serial,t,key,lo,hi,avg,n,crc=r
    return 0<serial<0xffffffff and 1609459200<=t<4102444800 and key>0 and n>0 and all(math.isfinite(v) for v in (lo,hi,avg)) and lo<=avg<=hi and crc==checksum(RECORD.pack(*r)[:28])

def recovered(path,expected):
    raw=path.read_bytes()
    if hashlib.sha256(raw).hexdigest()!=expected:raise ValueError('Recovered CSV SHA256 differs from trusted recovery')
    result={tier:[] for tier in TIERS}
    for row in csv.DictReader(raw.decode('utf-8-sig').splitlines()):
        tier=row['tier'];t=int(row['unix_epoch']);key=int(row['metric_hash_hex'],16)
        values=[float(row[x]) for x in ('min','max','average')];n=int(row['sample_count'])
        record=(1,t,key,*values,n,0);packed=RECORD.pack(*record);record=RECORD.unpack(packed[:28]+struct.pack('<I',checksum(packed[:28])))
        if not valid(record) or tier not in result:raise ValueError('Invalid recovered row')
        result[tier].append(record)
    return result

def prepare(csv_path,sha,current=None,output=None):
    old=recovered(csv_path,sha)
    report={'recoveredRows':sum(map(len,old.values())),'tiers':{k:len(v) for k,v in old.items()},'csvSha256':sha,'currentBackedUp':False,'installed':False}
    if current is None:return report
    if output is None or output.exists():raise ValueError('Output must name a new directory')
    candidates={};originals={};conflicts=0;corrupt=0;duplicates=0
    for tier,(name,capacity) in TIERS.items():
        path=current/name
        if not path.is_file():raise ValueError('Require all three backed-up journals: '+name)
        data=path.read_bytes();originals[name]=data;records=[]
        for offset in range(0,len(data)-RECORD.size+1,RECORD.size):
            r=RECORD.unpack_from(data,offset)
            if valid(r):records.append(r)
            else:corrupt+=1
        unique={};by_time={}
        for r in records+old[tier]:
            # Exact values are rounded to the deployed float32 representation.
            ident=identity(r)
            if ident in unique:duplicates+=1;continue
            time_key=(r[1],r[2])
            # Identical aggregate time/key with differing values can overlap;
            # summing counts would invent knowledge about their provenance.
            if tier!='recent' and time_key in by_time:conflicts+=1
            by_time[time_key]=r;unique[ident]=r
        values=sorted(unique.values(),key=lambda r:(r[1],r[2],r[7]))
        if len(values)>capacity:raise ValueError('Merge exceeds '+tier+' capacity; explicit retention decision needed')
        out=bytearray()
        for serial,r in enumerate(values,1):
            packed=RECORD.pack(serial,*r[1:7],0);out+=packed[:28]+struct.pack('<I',checksum(packed[:28]))
        candidates[name]=bytes(out)
    if conflicts:raise ValueError(f'{conflicts} overlapping aggregate conflicts require reconciliation; no files written')
    output.mkdir(mode=0o700);(output/'originals').mkdir();(output/'candidates').mkdir()
    for name,data in originals.items():(output/'originals'/name).write_bytes(data)
    for name,data in candidates.items():(output/'candidates'/name).write_bytes(data)
    report.update(currentBackedUp=True,duplicatesRemoved=duplicates,corruptCurrentRecords=corrupt,
        originals={k:hashlib.sha256(v).hexdigest() for k,v in originals.items()},
        candidates={k:hashlib.sha256(v).hexdigest() for k,v in candidates.items()},
        rollback='Restore all three originals with logging paused; verify hashes before resuming.')
    (output/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report
if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--recovered',required=True,type=pathlib.Path);ap.add_argument('--sha256',required=True);ap.add_argument('--current',type=pathlib.Path);ap.add_argument('--output',type=pathlib.Path);a=ap.parse_args()
    print(json.dumps(prepare(a.recovered,a.sha256,a.current,a.output),indent=2))
