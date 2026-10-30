#!/usr/bin/env python3
"""Provision the optional T1N ESP32 microSD over its existing Wi-Fi web server.

The device MUST already run an SD-enabled firmware and be disconnected from
the vehicle diagnostic session. This does not touch flash partitions or NVS.
"""
import argparse
import hashlib
import io
import json
import pathlib
import urllib.error
import urllib.parse
import urllib.request
import uuid
import zipfile

FILES = ("www/index.html", "www/advanced.html", "www/pi.html", "www/manifest.json")


def request(base, path, data=None, headers=None, timeout=25):
    url = base.rstrip("/") + path
    method = "POST" if data is not None else "GET"
    req = urllib.request.Request(url, data=data, method=method, headers=headers or {})
    try:
        with urllib.request.urlopen(req, timeout=timeout) as res:
            return res.read()
    except urllib.error.HTTPError as exc:
        raise RuntimeError(f"{path}: HTTP {exc.code}: {exc.read()[:250]!r}") from exc


def upload(base, filename, data):
    boundary = "t1n" + uuid.uuid4().hex
    body = (
        ("--" + boundary + "\r\n"
         + 'Content-Disposition: form-data; name="asset"; filename="' + filename + '"\r\n'
         + "Content-Type: application/octet-stream\r\n\r\n").encode()
        + data + ("\r\n--" + boundary + "--\r\n").encode()
    )
    urlpath = "/api/sd/upload?name=" + urllib.parse.quote(filename)
    result = request(base, urlpath, body,
                     {"Content-Type": "multipart/form-data; boundary=" + boundary},
                     timeout=50)
    print("Upload:", result.decode("utf-8", "replace"))
    actual = request(base, "/api/sd/file?name=" + urllib.parse.quote(filename))
    if hashlib.sha256(actual).digest() != hashlib.sha256(data).digest():
        raise RuntimeError(filename + ": SD readback SHA256 mismatch")
    print("Verified:", filename, len(data), "bytes")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--base", default="http://192.168.8.168",
                    help="T1N scanner local base URL")
    ap.add_argument("--bundle", required=True, help="SD asset zip from the matching firmware release")
    ap.add_argument("--apply",action="store_true",help="Write assets only after review")
    ap.add_argument("--parked-confirmed",action="store_true",help="Operator confirms parked conditions and approved maintenance")
    ap.add_argument("--backup-dir",type=pathlib.Path,help="New private directory for old assets")
    args = ap.parse_args()

    status = json.loads(request(args.base, "/api/status"))
    if status.get("connected") or status.get("busy") or status.get("polling"):
        raise SystemExit("Disconnect the diagnostic module before writing to SD.")
    monitor = json.loads(request(args.base, "/api/monitor/status"))
    if monitor.get("enabled") is not False:
        raise SystemExit("Explicitly pause monitoring before provisioning; this script never changes its mode.")
    sd = json.loads(request(args.base, "/api/sd/status"))
    print("SD status:", json.dumps(sd, indent=2))
    if not sd.get("mounted"):
        raise SystemExit("microSD is not mounted: " + sd.get("error", "unknown reason"))

    with zipfile.ZipFile(args.bundle, "r") as archive:
        contents = {name: archive.read(name) for name in FILES}
    manifest = json.loads(contents["www/manifest.json"])
    if manifest.get("version") != status.get("firmware"):
        raise SystemExit("SD assets version differs from running firmware; refusing mismatched upload.")
    for name in FILES[:-1]:
        actual = hashlib.sha256(contents[name]).hexdigest()
        if manifest["sha256"].get(name) != actual:raise SystemExit("Bundle checksum invalid for " + name)

    if not args.apply:
        print("Bundle and device state checked; no device writes. Use --apply --parked-confirmed --backup-dir NEW_DIRECTORY after approval.")
        return
    if not args.parked_confirmed or not args.backup_dir or args.backup_dir.exists():
        raise SystemExit("Require parked confirmation and a new backup directory before writes")
    args.backup_dir.mkdir(mode=0o700,parents=True)
    for name in FILES:
        base=name.rsplit("/",1)[-1]
        present=next((f.get("present") for f in sd.get("files",[]) if f.get("name")==base),False)
        if present:(args.backup_dir/base).write_bytes(request(args.base,"/api/sd/file?name="+base))
    (args.backup_dir/"device-status.json").write_text(json.dumps({"status":status,"monitor":monitor,"sd":sd},indent=2))
    print(request(args.base, "/api/sd/test", data=b"").decode())
    # Upload HTML before manifest, so manifest is evidence that all files are verified.
    for name in FILES:
        upload(args.base, name.rsplit("/", 1)[-1], contents[name])
    sd = json.loads(request(args.base, "/api/sd/status"))
    if not all(f["present"] for f in sd["files"]):
        raise SystemExit("The SD card does not show all expected files.")
    home = request(args.base, "/")
    if hashlib.sha256(home).digest() != hashlib.sha256(contents["www/index.html"]).digest():
        raise SystemExit("Scanner '/' did not serve the expected SD dashboard.")
    pi=request(args.base,"/pi")
    if hashlib.sha256(pi).digest()!=hashlib.sha256(contents["www/pi.html"]).digest():
        raise SystemExit("Scanner /pi did not serve expected dashboard")
    print("SUCCESS: mobile and Pi pages verified from microSD. Old assets retained in",args.backup_dir)


if __name__ == "__main__":
    main()
