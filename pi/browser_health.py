"""Read-only Chromium renderer heartbeat using a private loopback DevTools port.

Uses only the Python standard library. No proxy, ECU commands or sensor storage.
"""
import base64, hashlib, json, os, socket, struct, time, urllib.parse, urllib.request

def read_exact(stream, size):
    result = bytearray()
    while len(result) < size:
        part = stream.read(size - len(result))
        if not part: raise OSError('Browser closed its diagnostics connection')
        result.extend(part)
    return bytes(result)

def write_all(stream, data):
    view = memoryview(data)
    while view:
        n = stream.write(view)
        if not n: raise OSError('Browser closed its diagnostics connection')
        view = view[n:]

def send_frame(stream, opcode, payload):
    mask = os.urandom(4)
    if len(payload) > 16384: raise ValueError('Browser message too large')
    header = bytes([0x80 | opcode])
    header += bytes([0x80 | len(payload)]) if len(payload) < 126 else bytes([0xfe]) + struct.pack('!H', len(payload))
    write_all(stream, header + mask + bytes(v ^ mask[i % 4] for i, v in enumerate(payload)))
    stream.flush()

def evaluate(ws_url, port):
    parsed = urllib.parse.urlsplit(ws_url)
    if parsed.scheme != 'ws' or parsed.hostname != '127.0.0.1' or parsed.port != port:
        raise ValueError('Browser diagnostics must stay on its private loopback port')
    key = base64.b64encode(os.urandom(16)).decode()
    with socket.create_connection(('127.0.0.1', port), timeout=2) as sock:
        sock.settimeout(2)
        with sock.makefile('rwb', buffering=0) as stream:
            write_all(stream, ('GET ' + parsed.path + ' HTTP/1.1\r\nHost: 127.0.0.1:' + str(port)
                          + '\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13'
                          + '\r\nSec-WebSocket-Key: ' + key + '\r\n\r\n').encode())
            headers = bytearray()
            while not headers.endswith(b'\r\n\r\n'):
                if len(headers) >= 8192: raise ValueError('Browser handshake too large')
                headers.extend(read_exact(stream, 1))
            accept = base64.b64encode(hashlib.sha1((key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest())
            if not headers.startswith(b'HTTP/1.1 101 ') or accept not in headers:
                raise ValueError('Invalid browser diagnostics handshake')
            expression = "({ready:document.readyState==='complete',content:!!document.body&&document.body.innerText.trim().length>0,error:!!document.querySelector('#main-frame-error')})"
            send_frame(stream, 1, json.dumps({'id':1,'method':'Runtime.evaluate','params':{'expression':expression,'returnByValue':True}}).encode())
            deadline = time.monotonic() + 3
            message = bytearray()
            while time.monotonic() < deadline:
                first, second = read_exact(stream, 2)
                length = second & 127
                if second & 128 or length == 127: raise ValueError('Invalid browser frame')
                if length == 126: length = struct.unpack('!H', read_exact(stream, 2))[0]
                if length > 16384 or len(message) + length > 16384: raise ValueError('Browser reply too large')
                payload = read_exact(stream, length); opcode = first & 15
                if opcode == 8: return False
                if opcode == 9: send_frame(stream, 10, payload); continue
                if opcode not in (0, 1): raise ValueError('Unexpected browser frame')
                message.extend(payload)
                if not first & 128: continue
                data = json.loads(message); message.clear()
                if data.get('id') != 1: continue
                value = data.get('result', {}).get('result', {}).get('value', {})
                return value.get('ready') is True and value.get('content') is True and value.get('error') is False
    return False

def browser_ready(profile, expected_url):
    try:
        port = int((profile / 'DevToolsActivePort').read_text().splitlines()[0])
        if not 1024 <= port <= 65535: return False
        # Ignore desktop proxy settings for the browser's local diagnostics.
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
        with opener.open('http://127.0.0.1:' + str(port) + '/json/list', timeout=2) as response:
            targets = json.loads(response.read(16385))
        target = next((t for t in targets if t.get('type') == 'page' and t.get('url') == expected_url), None)
        return bool(target) and evaluate(target['webSocketDebuggerUrl'], port)
    except (OSError, ValueError, KeyError, TypeError, StopIteration):
        return False
