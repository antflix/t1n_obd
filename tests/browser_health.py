#!/usr/bin/env python3
import base64, hashlib, http.server, importlib.util, json, pathlib, struct, tempfile, threading, unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('health',ROOT/'pi/browser_health.py');h=importlib.util.module_from_spec(spec);spec.loader.exec_module(h)
class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version='HTTP/1.1'
    healthy=True
    commands=[]
    def log_message(self,*_):pass
    def read_frame(self):
        a,b=h.read_exact(self.rfile,2);n=b&127
        if n==126:n=struct.unpack('!H',h.read_exact(self.rfile,2))[0]
        assert b&128,'Client frame must be masked'
        mask=h.read_exact(self.rfile,4);data=h.read_exact(self.rfile,n)
        return a&15,bytes(v^mask[i%4] for i,v in enumerate(data))
    def do_GET(self):
        if self.path=='/json/list':
            body=json.dumps([{'type':'page','url':'file:///offline.html','webSocketDebuggerUrl':'ws://127.0.0.1:'+str(self.server.server_port)+'/page'}]).encode()
            self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body);return
        key=self.headers['Sec-WebSocket-Key'];accept=base64.b64encode(hashlib.sha1((key+'258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest()).decode()
        self.send_response(101);self.send_header('Upgrade','websocket');self.send_header('Connection','Upgrade');self.send_header('Sec-WebSocket-Accept',accept);self.end_headers()
        op,data=self.read_frame();assert op==1
        command=json.loads(data);Handler.commands.append(command)
        self.wfile.write(b'\x89\x04ping');self.wfile.flush()
        assert self.read_frame()==(10,b'ping')
        body=json.dumps({'id':command['id'],'result':{'result':{'value':{'ready':Handler.healthy,'content':True,'error':False}}}}).encode()
        cut=len(body)//2
        self.wfile.write(bytes([1,cut])+body[:cut]+bytes([128,len(body)-cut])+body[cut:]);self.wfile.flush()
        self.close_connection=True

class HealthTest(unittest.TestCase):
    def test_actual_loopback_handshake_masking_ping_fragments_and_unready_page(self):
        server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler)
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        try:
            with tempfile.TemporaryDirectory() as d:
                p=pathlib.Path(d);(p/'DevToolsActivePort').write_text(str(server.server_port)+'\n/devtools/browser/fixture\n')
                Handler.healthy=True;self.assertTrue(h.browser_ready(p,'file:///offline.html'))
                Handler.healthy=False;self.assertFalse(h.browser_ready(p,'file:///offline.html'))
                self.assertFalse(h.browser_ready(p,'http://wrong-page'))
                self.assertTrue(all(x['method']=='Runtime.evaluate' and x['params']['returnByValue'] for x in Handler.commands))
        finally:server.shutdown();server.server_close();thread.join()
    def test_missing_profile_and_non_loopback_targets_fail(self):
        with tempfile.TemporaryDirectory() as d:self.assertFalse(h.browser_ready(pathlib.Path(d),'file:///offline.html'))
        with self.assertRaises(ValueError):h.evaluate('ws://192.168.8.168:9222/page',9222)
    def test_partial_writes_are_completed(self):
        class Writer:
            data=bytearray()
            def write(self,b):self.data.extend(b[:2]);return min(2,len(b))
        w=Writer();h.write_all(w,b'abcdefgh');self.assertEqual(w.data,b'abcdefgh')

if __name__=='__main__':unittest.main()
