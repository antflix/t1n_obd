# Restore the verified version24 baseline — 2026-10-31

The user explicitly requested removing introduced code rather than adding fixes
on top of the instability. The version27 shared JSON workspace candidate was
built and tested but never installed. The later proposed history-only deadline
extension was discarded from the working tree without committing or deploying.

## Source restoration

On `fix/restore-stable-v24-20261031`, restore `firmware/`, `scripts/` and `tests/`
exactly from version24 source commit
`eb90f0d575e2862e923fbc409f6ed5143bdcae0f`. A staged Git comparison of all three
directories against that commit is empty. The later runtime changes are removed,
including paired history requests and their second result buffer, the revised
automatic ownership/scheduler integration, and the version26/27 encoder changes.
Earlier experimental notes remain as history; they do not describe the active
runtime on this branch.

This is a rollback to a measured working baseline, not proof identifying which
of the bundled changes caused every outage. Version24 already has one shared
SD-backed history client and no browser-generated historical database. It loads
selected metrics with separate SD queries, so paired charts retain the older,
slower behavior rather than the larger combined response.

The full version24 `python3 scripts/checks.py` suite passes, including native
history failure/recovery/rollover checks, streamed sensor JSON larger than 8 KB,
12 shared-client tests, mobile/Pi fixture checks, and read-only navigation/source
contracts. Tests are restored with the runtime they test; removed-feature tests
are not presented as applicable to this baseline.

## Retained verified artifact

- Firmware: `1.10.24-history-read-test`.
- Image size: 1,406,016 bytes; OTA partition: 1,966,080 bytes.
- SHA-256: `aa1b4b27fa9a55581861f47da79127d9445b7e9c37884658f99cac1796357ced`.
- Matching SD bundle SHA-256:
  `77e510ab9c8f6101175597d35ddaf3fc9f1de1f9b4527620eedb7d798102b3ae`.
- Previously verified long run: approximately 10.5 hours with 13,210 successful
  writes and no SD write errors; this is prior evidence, not the duration of the
  new rollback test.

The private image includes network configuration and is not published. No flash
backup, NVS erase, SD formatting, journal migration or unrelated-file change is
needed. Install the exact retained image and its matching four UI assets, then
verify installed version, asset hashes, sensor JSON and successful SD writes.

## Operating mode and qualification boundary

Restore the former engine-connected, polling configuration with automatic
monitoring disabled. The engine session still records to SD without an open
browser. This deliberately removes new automatic module visits from the live
comparison. It is not a claim that the old automatic ownership/hold or ABS policy
satisfies the final architecture. Do not enable this older unattended scheduler
or unattended ABS as part of the rollback verification.

The Pi is now accessible and has no competing scanner backend. Its active
`sprinter-dual-kiosk.sh` opens the separate navigation HUD and the correct scanner
URL, `http://192.168.8.111/pi`. Preserve both tabs, touchscreen mapping and SSH.
Reload only the scanner tab after the matching UI is installed.

At 20:37 UTC both HA and Pi could not reach the scanner; Pi SSH remained
available and its existing scanner tab reported offline. Neither computer has an
ESP32 enumerated on USB. The first rollback attempt failed at its initial GET,
before sending any pause, firmware or asset request. A single-attempt installer
was then queued to start when scanner HTTP returns. Installation and hardware
results must be recorded after actual completion; no success is inferred from
source restoration or a queued process.
