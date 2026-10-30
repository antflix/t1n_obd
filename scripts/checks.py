#!/usr/bin/env python3
"""Offline checks for shared SD data; never contacts hardware or a vehicle."""
import json,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def run(*args,**kw):return subprocess.run(args,cwd=ROOT,check=True,**kw)
with tempfile.TemporaryDirectory(prefix='t1n-shared-checks-') as folder:
    for name in ['monitor_dtc_diff','history_storage','bounded_json']:
        binary=str(pathlib.Path(folder)/name)
        run('g++','-std=c++17','-Wall','-Wextra','-Werror','-Itests/fakes','-Ifirmware/t1n_obd','tests/'+name+'.cpp','-o',binary)
        if name=='bounded_json':
            body=run(binary,capture_output=True).stdout
            data=json.loads(body)
            assert len(body)>8192 and len(data['items'])==500 and data['invalid'] is None
            assert data['items'][0]['label']=='quoted" \\ line\n tab\t CR\r \001 °C'
            print('Large streaming JSON (>8 KB), control escaping and bounded chunks PASS')
        else:run(binary)
for test in ['shared_client','mobile_view','pi_view']:
    run('node','tests/'+test+'.cjs')
run(sys.executable,'tests/shared_source.py')
print('Offline shared SD data checks PASS')
