"""Integration contracts: one source for recorded data, assets and sensor enumeration."""
import importlib.util,pathlib,re,json,hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
def read(name):return (ROOT/name).read_text()
history=read('firmware/t1n_obd/36_history.ino')
mobile=read('firmware/t1n_obd/50_scanner_ui.ino')
pi=read('firmware/sdcard/pi.html')
client=read('firmware/sdcard/t1n-data.js')
routes=read('firmware/t1n_obd/90_setup.ino')
sensors=read('firmware/t1n_obd/30_modules.ino')
for obsolete in ['SPIFFS','hUsingSD','hFs','historyDataJson']:
    assert obsolete not in history,obsolete
for obsolete in ['rememberItems','seenSamples','multiBrowserPoints','function multiSeries','trendRange="live"','source:\'browser\'','Flash history']:
    assert obsolete not in mobile,obsolete
for page in [mobile,pi]:assert '<script src="/t1n-data.js"></script>' in page
assert 'web.sendContent' not in sensors and 'web.sendContent' not in history
assert 'sendScannerJson' in sensors and 'sendScannerJson' in history
assert 'MSG_DONTWAIT' in read('firmware/t1n_obd/http_scanner.h')
assert 'sensorsJson' not in sensors and 'String valueJson(' not in sensors
assert 'template <typename Add> void forEachSensor' in sensors
assert 'void historyObserveSensors()' in sensors and 'void sensorsSend()' in sensors
assert 'historyQueryTick();' in routes and 'historyDataSend(' in routes
assert '/api/module/select' not in routes
spec=importlib.util.spec_from_file_location('package_web',ROOT/'scripts/package_web.py')
package=importlib.util.module_from_spec(spec);spec.loader.exec_module(package)
assets=package.assets();manifest=json.loads(assets['www/manifest.json'])
assert set(assets)=={'www/index.html','www/advanced.html','www/pi.html','www/manifest.json'}
assert manifest['historySchema']==1
for name,digest in manifest['sha256'].items():assert hashlib.sha256(assets[name]).hexdigest()==digest
assert assets['www/pi.html']==(ROOT/'firmware/sdcard/pi.html').read_bytes()
print('One SD history source, canonical sensor visitor, matching UI package and read-only module browsing PASS')
