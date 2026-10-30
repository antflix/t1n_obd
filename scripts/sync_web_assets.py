#!/usr/bin/env python3
"""Keep SD pages, shared client, firmware fallback and checksums together."""
import argparse,hashlib,json,pathlib,re,zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def sync(check=False):
    shared=(ROOT/'firmware/web/monitor-client.js').read_text().strip()
    block='/* T1N_SHARED_BEGIN */\n'+shared+'\n/* T1N_SHARED_END */'
    outputs={}
    for rel in ['firmware/t1n_obd/50_scanner_ui.ino','firmware/sdcard/www/pi.html']:
        path=ROOT/rel;s=path.read_text()
        if '/* T1N_SHARED_BEGIN */' in s:
            s=re.sub(r'/\* T1N_SHARED_BEGIN \*/.*?/\* T1N_SHARED_END \*/',lambda _:block,s,flags=re.S)
        else:s=re.sub(r'(<script[^>]*>)',lambda m:m[1]+'\n'+block+'\n',s,count=1)
        outputs[path]=s
    for rel,fn,name in [('firmware/t1n_obd/50_scanner_ui.ino','scannerPage','index.html'),('firmware/t1n_obd/40_advanced_ui.ino','advancedPage','advanced.html')]:
        path=ROOT/rel;s=outputs.get(path,path.read_text())
        m=re.search(r'String '+fn+r'\(\)\{return R"HTML\((.*?)\)HTML";\}',s,re.S)
        if not m:raise ValueError('Cannot extract '+fn)
        outputs[ROOT/'firmware/sdcard/www'/name]=m[1]
    # Preserve the historical source path as a generated copy, never a second implementation.
    outputs[ROOT/'firmware/sdcard/pi.html']=outputs[ROOT/'firmware/sdcard/www/pi.html']
    version=re.search(r'FIRMWARE_VERSION\s*=\s*"([^"]+)"',(ROOT/'firmware/t1n_obd/t1n_obd.ino').read_text())[1]
    assets={p.name:s.encode() for p,s in outputs.items() if p.parent.name=='www'}
    manifest={'version':version,'pinout':{'cs':22,'clk':18,'miso':19,'mosi':17},'sha256':{'www/'+name:hashlib.sha256(value).hexdigest() for name,value in sorted(assets.items())}}
    outputs[ROOT/'firmware/sdcard/www/manifest.json']=json.dumps(manifest,indent=2)+'\n'
    for p,s in outputs.items():
        if check:
            if p.read_text()!=s:raise SystemExit('Stale generated web asset: '+str(p))
        else:p.write_text(s)
    return outputs
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');parser.add_argument('--zip',type=pathlib.Path);args=parser.parse_args()
    sync(args.check)
    if args.zip:
        args.zip.parent.mkdir(parents=True,exist_ok=True)
        with zipfile.ZipFile(args.zip,'w',zipfile.ZIP_DEFLATED) as z:
            for path in sorted((ROOT/'firmware/sdcard/www').glob('*')):z.write(path,'www/'+path.name)
