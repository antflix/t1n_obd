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

## Actual installation and failed qualification

The queued single attempt subsequently succeeded: OTA returned HTTP200
`Firmware installed. Rebooting...`; the installed version was checked as
`1.10.24-history-read-test`. All four SD assets passed SHA-256 readback. The
engine connected and polling started. At uptime 8,710 ms, status reported 96,964
free heap bytes, 3,804 minimum stack bytes, zero HTTP errors/timeouts and 58 ms
maximum JSON send time. Automatic monitoring remained disabled as intended.

This did **not** qualify the rollback as stable. The Pi completed its 19119-byte
`/pi` document in 171 ms but remained blocked on `/t1n-data.js`; both computers
then lost HTTP and ARP access before the parent's chart test started. The
initial passive SD-write check could not complete. Thus removing versions25–27
alone did not eliminate the observed failure.

## Serial capture with the ESP32 attached to Pi USB

The user connected the scanner's CP2102 USB bridge to the Pi. Capture used the
existing 115200-baud console, without sending serial commands or flashing any
firmware. The capture contains a normal `POWERON_RESET` boot at 20:50:22 UTC and
no later panic, backtrace, brownout or watchdog message during its ten minutes.
USB/serial attachment can affect reset and power conditions; do not equate this
boot with an unexplained spontaneous reset. The user reports the prior supply
was a 12V-to-5V buck converter; its relationship to the USB supply still matters.

With diagnostics disconnected, the Pi loaded fully in 250 ms; its shared script
completed in approximately 20 ms and 30 seconds of GET-only polling passed.
One host GET also received the 16116-byte shared script in 38 ms. The parent then
connected only the engine and retained the same USB connection and firmware.
An initial 60-second check showed 154→275 successful SD writes, zero SD errors,
continuous uptime and an uninterrupted engine session.

The ordinary mobile chart check failed: rail-target history and sensor requests
returned `ERR_CONTENT_LENGTH_MISMATCH`; the UI correctly reported a history
error rather than substituting browser data. Pi polling also showed five content
length mismatches during one 60-second observation. Status remained connected,
with 17 HTTP send timeouts, 1,532 minimum stack bytes and no reset at uptime 378,556
ms. This independently confirms the 250 ms absolute-send cutoff remains a chart
and live-data problem in version24. No deadline extension was installed.

Around 20:59 UTC, HTTP/ping/ARP at the station address failed again while the
existing serial log continued normal engine TX/RX through 21:00:22 UTC and
diagnostic uptime 599,496 ms. Therefore this observed outage did not stop the
application or require a processor reset to explain it. It does not establish
whether the remaining failure is Wi-Fi association, router forwarding, or the
ESP32 networking stack. Check the private serial summary for transaction counts
and measured request timing; the configured 300 ms pause was not changed.

At 21:06:25 UTC, a fresh Pi scan still saw `T1N-Scanner`, BSSID
`20:50:0d:d2:9c:a5`, at -43 dBm on channel 2. The AP beacon proves radio activity,
not functioning IP or HTTP. The Pi stayed associated to AntNet and its settings,
HUD tab, SSH and touchscreen were preserved. Direct AP access from the user's
phone was requested without resetting the scanner, to distinguish the station
path from the shared IP/HTTP implementation. That result is still pending.

Raw console output and build images remain private. Runtime source stays exactly
version24 on this branch; the failure has not been hidden by a new retry,
fallback, forced reset, changed polling interval or automatic reconnection layer.
