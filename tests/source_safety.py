#!/usr/bin/env python3
"""Route and packaging guards complement behavior tests; no hardware requests."""
import pathlib,re,subprocess,tempfile
R=pathlib.Path(__file__).resolve().parents[1]
s=(R/'firmware/t1n_obd/90_setup.ino').read_text()
paths=['/connect','/disconnect','/poll/start','/poll/stop','/dtc/read','/dtc/clear','/payload','/api/module/select','/api/module/connect','/api/module/read','/api/disconnect','/api/poll/start','/api/poll/stop','/api/dtc/read','/api/relay/test','/api/shared-probe/run','/api/shared-probe/timing','/api/hzr/fan/on','/api/hzr/fan/status','/api/test','/api/test/actuator','/api/test/release','/config/save','/config/defaults','/config/importlab']
for path in paths:
    assert 'web.on("'+path+'",HTTP_POST,[]{if(rejectAutomaticOwner())return;' in s,path
assert 'web.send(409,"application/json"' in s
for rel in ['index.html','pi.html']:
    html=(R/'firmware/sdcard/www'/rel).read_text()
    assert '/* T1N_SHARED_BEGIN */' in html
    assert 'seriesCache' not in html and 'browser live samples' not in html and 'Flash history' not in html
    for script in re.findall(r'<script[^>]*>(.*?)</script>',html,re.S):
        with tempfile.NamedTemporaryFile(mode='w',suffix='.js') as f:
            f.write(script);f.flush();subprocess.run(['node','--check',f.name],check=True)
assert 'monAbsChannelQualified=false' in (R/'firmware/t1n_obd/38_autonomous_monitor.ino').read_text()
print('25 manual route guards, dashboard syntax, shared asset and ABS gates PASS')

guard=re.search(r'bool rejectAutomaticOwner\(\)\{(.*?)\n\}',s,re.S)[0]
wrapper='#include <cassert>\n#include <string>\nbool monEnabled=false;struct Web{int status=0;std::string body;void sendHeader(const char*,const char*){}void send(int s,const char*,const char*b){status=s;body=b;}}web;\n'+guard+'\nint main(){assert(!rejectAutomaticOwner());monEnabled=true;assert(rejectAutomaticOwner());assert(web.status==409);assert(web.body.find("AUTOMATIC_UART_OWNER")!=std::string::npos);}'
with tempfile.TemporaryDirectory() as d:
    path=pathlib.Path(d)/'guard.cpp';path.write_text(wrapper);out=str(pathlib.Path(d)/'guard')
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',str(path),'-o',out],check=True);subprocess.run([out],check=True)
print('Actual automatic-owner guard returns 409 and permits explicit manual mode PASS')

transport=(R/'firmware/t1n_obd/10_transport.ino').read_text()
helper=transport[transport.index('void appendDiagnosticLog('):transport.index('void activity(')]
wrapper='#include <cassert>\n#include "FS.h"\nuint32_t diagnosticLogAllocationErrors=0;\n'+helper+'\nint main(){String log;for(int i=0;i<5000;i++){appendDiagnosticLog(log,String("Diagnostic fixture line"),256);assert(log.length()<=256);}assert(diagnosticLogAllocationErrors==0);}'
with tempfile.TemporaryDirectory() as d:
    path=pathlib.Path(d)/'log.cpp';path.write_text(wrapper);out=str(pathlib.Path(d)/'log')
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror','-I',str(R/'tests/fakes'),str(path),'-o',out],check=True);subprocess.run([out],check=True)
print('Actual bounded diagnostic log remains within capacity after 5000 appends PASS')

assert (R/'firmware/sdcard/pi.html').read_bytes()==(R/'firmware/sdcard/www/pi.html').read_bytes()
print('Legacy Pi source path matches the canonical SD page PASS')

assert 'WiFi.setAutoReconnect(false);' in s and 'WiFi.setAutoReconnect(true)' not in s
assert 'lastWifiRetryMs' not in s
assert s.count('WiFi.begin(wifiSSID.c_str(),wifiPASS.c_str())')==2
assert s.count('WiFi.disconnect(false,false)')==2
assert 'wifiRecovery.settingsChanged(millis())' in s and 'wifiTick();delay(2)' in s
print('Single Wi-Fi retry owner, AP-preserving disconnect and deferred settings reconnect PASS')
