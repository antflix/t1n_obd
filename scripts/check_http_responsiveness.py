#!/usr/bin/env python3
"""Read-only regression: a non-draining HTTP reader must not strand the ESP32.

Runs only GET requests; never changes ECU sessions or scanner configuration.
This deliberately stalls one socket and requires a firmware with bounded output.
"""
import argparse
import json
import socket
import time
import urllib.parse
import urllib.request


def get(base, path):
    started = time.monotonic()
    with urllib.request.urlopen(base + path, timeout=2) as response:
        data = json.load(response)
    return data, time.monotonic() - started


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', required=True)
    parser.add_argument('--repeat', type=int, default=3)
    args = parser.parse_args()
    base = args.base.rstrip('/')
    address = urllib.parse.urlsplit(base)
    if address.scheme != 'http' or not address.hostname:
        raise SystemExit('Expected a local http:// scanner URL')
    before, _ = get(base, '/api/status')
    if 'httpTimeouts' not in before:
        raise SystemExit('Scanner does not expose bounded HTTP counters')
    results = []
    for _ in range(args.repeat):
        stalled = socket.socket()
        stalled.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
        if hasattr(socket, 'TCP_WINDOW_CLAMP'):
            stalled.setsockopt(socket.IPPROTO_TCP, socket.TCP_WINDOW_CLAMP, 1024)
        stalled.settimeout(2)
        try:
            stalled.connect((address.hostname, address.port or 80))
            stalled.sendall(('GET /api/sensors HTTP/1.1\r\nHost: ' + address.netloc +
                             '\r\nConnection: close\r\n\r\n').encode())
            # Leave the socket open and do not drain its advertised receive window.
            time.sleep(.6)
            status, elapsed = get(base, '/api/status')
            assert elapsed < 1, f'Stalled reader delayed health request: {elapsed:.3f}s'
            assert status['uptimeMs'] >= before['uptimeMs'], 'Scanner reset during stall'
            results.append({'statusSeconds': round(elapsed, 3), 'uptimeMs': status['uptimeMs'],
                            'httpTimeouts': status['httpTimeouts'], 'httpMaxSendMs': status['httpMaxSendMs']})
        finally:
            stalled.close()
    sensors, _ = get(base, '/api/sensors')
    assert isinstance(sensors['items'], list), 'Sensor endpoint did not recover'
    after, _ = get(base, '/api/status')
    assert after['httpTimeouts'] > before['httpTimeouts'], 'Test did not exercise a blocked response'
    print(json.dumps({'firmware': after['firmware'], 'results': results,
                      'timeoutsAdded': after['httpTimeouts'] - before['httpTimeouts'],
                      'sensorItems': len(sensors['items'])}))


if __name__ == '__main__':
    main()
