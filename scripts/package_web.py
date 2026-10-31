#!/usr/bin/env python3
"""Build one matching SD UI bundle and the embedded shared-client recovery asset."""
import argparse, hashlib, json, pathlib, re, zipfile
ROOT=pathlib.Path(__file__).resolve().parents[1]

def assets():
    contents={}
    for file,function,name in [('50_scanner_ui.ino','scannerPage','index.html'),('40_advanced_ui.ino','advancedPage','advanced.html')]:
        source=(ROOT/'firmware/t1n_obd'/file).read_text()
        match=re.search(r'String '+function+r'\(\)\{return R"HTML\((.*?)\)HTML";\}',source,re.S)
        if not match:raise ValueError('Missing page: '+file)
        contents['www/'+name]=match[1].encode()
    contents['www/pi.html']=(ROOT/'firmware/sdcard/pi.html').read_bytes()
    version=re.search(r'FIRMWARE_VERSION\s*=\s*"([^"]+)"',(ROOT/'firmware/t1n_obd/t1n_obd.ino').read_text())[1]
    manifest={'version':version,'historySchema':1,'pinout':{'cs':22,'clk':18,'miso':19,'mosi':17},
              'sha256':{name:hashlib.sha256(data).hexdigest() for name,data in sorted(contents.items())}}
    contents['www/manifest.json']=(json.dumps(manifest,indent=2)+'\n').encode()
    return contents

def generate():
    client=(ROOT/'firmware/sdcard/t1n-data.js').read_text()
    assert ')T1NCLIENT"' not in client
    header='#pragma once\n#include <Arduino.h>\nstatic const char T1N_SHARED_CLIENT[] PROGMEM = R"T1NCLIENT('+client+')T1NCLIENT";\n'
    (ROOT/'firmware/t1n_obd/shared_client_generated.h').write_text(header)

if __name__=='__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--zip',type=pathlib.Path)
    args=ap.parse_args();generate();contents=assets()
    if args.zip:
        args.zip.parent.mkdir(parents=True,exist_ok=True)
        with zipfile.ZipFile(args.zip,'w',zipfile.ZIP_DEFLATED) as z:
            for name,data in sorted(contents.items()):z.writestr(name,data)
    print('Shared client and matching SD pages generated:',', '.join(contents))
