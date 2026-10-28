#!/usr/bin/env python3
"""Safe two-stage T1N ESP32 USB migration.

By default this tool ONLY backs up the original 4 MiB flash (includes NVS,
Wi-Fi credentials, history, OTA). Flashing requires explicit --flash and a
matching verified release, and preserves the NVS partition in place.

Use esptool 4.x (for example python3 -m pip install esptool==4.9.0).
Connect the ESP32 by USB and ensure it is not connected to a vehicle bus
during USB flashing. Never expose backup images: they contain credentials.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import time

CAPACITY = 0x400000
PT_OFFSET = 0x8000
OTA_DATA_OFFSET = 0xE000


def run(port, *args):
    command = [sys.executable, "-m", "esptool", "--chip", "esp32",
               "--port", port, "--baud", "460800", *map(str, args)]
    print("COMMAND:", " ".join(command), flush=True)
    subprocess.run(command, check=True)


def partitions(table):
    result = {}
    for i in range(0, len(table) - 32, 32):
        row = table[i:i+32]
        if row[:2] != b"\xaa\x50":
            continue
        magic, kind, sub, offset, size, label, flags = struct.unpack("<HBBII16sI", row)
        name = label.split(b"\0", 1)[0].decode("ascii", "replace")
        result[name] = dict(type=kind, subtype=sub, offset=offset, size=size, flags=flags)
    return result


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="Verified ESP32 USB serial port")
    parser.add_argument("--backup-dir", default=str(Path.home() / "t1n-flash-backups"))
    parser.add_argument("--release-dir", help="Directory with the matching GitHub release binaries")
    parser.add_argument("--flash", action="store_true",
                        help="DANGEROUS: migrate partitions and install firmware after local backup")
    args = parser.parse_args()
    backup_dir = Path(args.backup_dir).expanduser()
    backup_dir.mkdir(parents=True, exist_ok=True)
    os.chmod(backup_dir, 0o700)

    run(args.port, "flash_id")
    stamp = time.strftime("%Y%m%d-%H%M%S")
    backup = backup_dir / f"t1n-before-sd-migration-{stamp}-4MiB.bin"
    run(args.port, "read_flash", "0x0", hex(CAPACITY), backup)
    if backup.stat().st_size != CAPACITY:
        raise RuntimeError("Backup is not 4 MiB; flashing aborted.")
    os.chmod(backup, 0o600)
    print("BACKUP:", backup, "SHA256:", sha256(backup))
    previous = partitions(backup.read_bytes()[PT_OFFSET:PT_OFFSET+4096])
    print("OLD_PARTITIONS:", json.dumps(previous, indent=2))

    if not args.flash:
        print("BACKUP COMPLETE. No flash memory was modified.")
        return
    if not args.release_dir:
        raise RuntimeError("--release-dir required with --flash.")
    release = Path(args.release_dir).expanduser()
    required = ("manifest.json", "firmware.bin", "bootloader.bin", "partitions.bin")
    for name in required:
        if not (release/name).is_file():
            raise RuntimeError("Missing release asset: " + name)
    manifest = json.loads((release/"manifest.json").read_text())
    app = release/"firmware.bin"
    if sha256(app) != manifest["sha256"]:
        raise RuntimeError("Firmware SHA256 does not match the published manifest")
    new = partitions((release/"partitions.bin").read_bytes())
    print("NEW_PARTITIONS:", json.dumps(new, indent=2))
    if not previous.get("nvs") or not new.get("nvs"):
        raise RuntimeError("NVS partition missing; refusing flash")
    for field in ("offset", "size"):
        if previous["nvs"][field] != new["nvs"][field]:
            raise RuntimeError("NVS partition moved/resized; refusing flash")
    if "app0" not in new or new["app0"]["offset"] != 0x10000:
        raise RuntimeError("Unexpected new app0 partition")
    if "app1" not in new or new["app1"]["size"] < 0x1C0000:
        raise RuntimeError("The release does not have two large OTA app slots")
    if app.stat().st_size > new["app0"]["size"]:
        raise RuntimeError("Firmware does not fit app0")
    if not (release/"bootloader.bin").stat().st_size < 65536:
        raise RuntimeError("Unexpected bootloader size")
    # Write only bootloader, partition table and app0. NVS remains intact.
    # Finally clear old otadata so the new bootloader starts app0.
    print("FLASHING: immutable backup exists. This replaces the current layout.")
    run(args.port, "write_flash", "--flash_mode", "dio", "--flash_freq", "40m",
        "--flash_size", "4MB",
        "0x1000", release/"bootloader.bin",
        "0x8000", release/"partitions.bin",
        "0x10000", app)
    run(args.port, "erase_region", hex(OTA_DATA_OFFSET), "0x2000")
    print("USB FLASH COMPLETE. Confirm installed version via /api/status.")
    print("The prior SPIFFS history is preserved in the private flash backup,")
    print("but the new partition map may require migrating it to the SD card.")


if __name__ == "__main__":
    main()
