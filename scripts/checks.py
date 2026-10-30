#!/usr/bin/env python3
"""Offline regression suite; never sends requests to the scanner or vehicle."""
import json,pathlib,subprocess,sys,tempfile
ROOT=pathlib.Path(__file__).resolve().parents[1]
def run(*args,**kw):return subprocess.run(args,cwd=ROOT,check=True,**kw)
with tempfile.TemporaryDirectory(prefix='t1n-tests-') as d:
    for name in ['monitor_dtc_diff','monitor_ignition_gate','monitor_retry_policy','history_storage','alert_rule','alert_persistence','bounded_json']:
        out=str(pathlib.Path(d)/name)
        run('g++','-std=c++17','-Wall','-Wextra','-Werror','-I','tests/fakes','-I','firmware/t1n_obd','tests/'+name+'.cpp','-o',out)
        if name=='bounded_json':
            body=run(out,capture_output=True).stdout;data=json.loads(body)
            assert len(body)>8192 and len(data['items'])==500 and data['invalid'] is None
            assert data['items'][0]['label']=='quoted" \\ line\n tab\t CR\r \001 °C'
            print('Large streaming JSON and control escaping PASS')
        else:run(out)
run(sys.executable,'scripts/sync_web_assets.py','--check')
run(sys.executable,'tests/source_safety.py')
run(sys.executable,'tests/kiosk_supervisor.py')
run(sys.executable,'tests/history_merge.py')
run('node','tests/shared_client.cjs')
print('Offline regression suite PASS')
