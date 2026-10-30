#!/usr/bin/env python3
import importlib.util,pathlib,tempfile,unittest,sys
from unittest.mock import patch
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'pi'))
spec=importlib.util.spec_from_file_location('kiosk',pathlib.Path(__file__).resolve().parents[1]/'pi/esp-kiosk.py');k=importlib.util.module_from_spec(spec);spec.loader.exec_module(k)
class Child:
    def __init__(self):self.dead=False
    def poll(self):return 0 if self.dead else None
    def terminate(self):self.dead=True
    def wait(self,timeout=None):return 0
class KioskTest(unittest.TestCase):
    def test_offline_start_recovery_and_no_duplicate_browser(self):
        calls=[];iterations=0;children=[]
        def launch(args):
            calls.append(args);c=Child();children.append(c);return c
        def reachable(url):
            nonlocal iterations
            if url==k.HEALTH:iterations+=1
            if iterations>4:raise KeyboardInterrupt
            return iterations in (2,4)
        with tempfile.TemporaryDirectory() as d,patch.object(k,'PROFILE',pathlib.Path(d)),patch.object(k,'reachable',reachable),patch.object(k.subprocess,'Popen',launch),patch.object(k.time,'sleep'),patch.object(k.signal,'signal'):
            with self.assertRaises(KeyboardInterrupt):k.main()
        self.assertEqual(len(calls),3)
        for args in calls:
            self.assertIn('--password-store=basic',args)
            self.assertIn('--ozone-platform=wayland',args)
            self.assertIn('--disable-gpu',args)
            self.assertIn('--remote-debugging-address=127.0.0.1',args)
            self.assertIn('--remote-debugging-port=0',args)
            self.assertEqual(sum(a.startswith('--user-data-dir=') for a in args),1)
            self.assertIn('--kiosk',args)
        self.assertTrue(calls[0][-1].endswith('offline.html'));self.assertEqual(calls[1][-1],k.URL);self.assertEqual(calls[2][-1],k.URL)
        self.assertTrue(all(c.dead for c in children))
    def test_failure_is_not_success(self):
        with patch.object(k.urllib.request,'urlopen',side_effect=OSError('offline')):self.assertFalse(k.reachable(k.HEALTH))
    def test_live_process_with_frozen_renderer_is_restarted(self):
        calls=[];clock=[0];iterations=[0]
        def reachable(_):
            iterations[0]+=1
            if iterations[0]>13:raise KeyboardInterrupt
            return False
        def sleep(n):clock[0]+=n
        with tempfile.TemporaryDirectory() as d,patch.object(k,'PROFILE',pathlib.Path(d)),patch.object(k,'reachable',reachable),patch.object(k,'browser_ready',return_value=False),patch.object(k.subprocess,'Popen',side_effect=lambda a:(calls.append(a) or Child())),patch.object(k.time,'sleep',sleep),patch.object(k.time,'monotonic',side_effect=lambda:clock[0]),patch.object(k.signal,'signal'):
            with self.assertRaises(KeyboardInterrupt):k.main()
        self.assertEqual(len(calls),2,'Recover a stuck renderer after grace and three failures, without rapid relaunch')
if __name__=='__main__':unittest.main()
