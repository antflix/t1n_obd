#!/usr/bin/env python3
"""Single Chromium display, local offline screen, bounded scanner recovery.
No ECU commands, proxy server, or measurement database. Preserve labwc/touch config.
"""
import fcntl,os,pathlib,signal,subprocess,time,urllib.request
URL=os.environ.get('T1N_SCANNER_URL','http://192.168.8.168/pi')
HEALTH=URL.rsplit('/',1)[0]+'/api/status'
PROFILE=pathlib.Path.home()/'.cache/t1n-kiosk-profile'
OFFLINE=pathlib.Path(__file__).with_name('offline.html')
BROWSER=os.environ.get('T1N_KIOSK_BROWSER','/usr/bin/chromium')
def reachable(url):
    try:
        with urllib.request.urlopen(url,timeout=3) as r:return r.status==200
    except (OSError,ValueError):return False

def main():
    PROFILE.mkdir(parents=True,exist_ok=True)
    lock=(PROFILE/'supervisor.lock').open('w')
    try:fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    except BlockingIOError:lock.close();return
    child=None;running=True;was_online=False;showing_remote=False
    def stop(*_):
        nonlocal running
        running=False
    signal.signal(signal.SIGTERM,stop);signal.signal(signal.SIGINT,stop)
    def launch(url):
        nonlocal child
        if child and child.poll() is None:
            child.terminate()
            try:child.wait(timeout=5)
            except subprocess.TimeoutExpired:child.kill();child.wait()
        child=subprocess.Popen([BROWSER,'--kiosk','--no-first-run','--no-default-browser-check',
            '--noerrdialogs','--disable-session-crashed-bubble','--overscroll-history-navigation=0',
            # Display-only profile: avoid a locked desktop keyring blocking boot.
            # Never use this separate, unencrypted profile for saved credentials.
            # The deployed Pi 3B EGL path produced a white screen; software
            # rendering is verified on its existing labwc/DSI configuration.
            '--password-store=basic','--ozone-platform=wayland','--disable-gpu',
            '--force-device-scale-factor=1','--user-data-dir='+str(PROFILE),url])
    launch(OFFLINE.as_uri())
    try:
        while running:
            online=reachable(HEALTH)
            if online and (not was_online or not showing_remote):
                if reachable(URL):launch(URL);showing_remote=True
            if child.poll() is not None:
                launch(URL if online and showing_remote else OFFLINE.as_uri())
            was_online=online
            # The loaded app handles routine outages. Relaunch once on recovery
            # also recovers Chromium's own network-error page after a startup race.
            for _ in range(10):
                if not running:break
                time.sleep(1)
    finally:
        if child and child.poll() is None:
            child.terminate()
            try:child.wait(timeout=5)
            except subprocess.TimeoutExpired:child.kill();child.wait()
        lock.close()
if __name__=='__main__':main()
