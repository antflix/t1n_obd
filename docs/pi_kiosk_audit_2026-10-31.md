# Pi kiosk audit — 2026-10-31

Status: scanner dashboard endpoint verified; physical Pi inspection pending because
its last-known address is unreachable. No Pi configuration, process, service,
driver or touchscreen mapping was changed.

## Read-only observations

- Development computer is on `192.168.8.250/24`; the local LAN route is present.
- SSH to `ant@192.168.8.136` using the existing dashboard key failed with
  `No route to host`. The local neighbor entry for that address is `FAILED`.
  Connection and keepalive timeouts were bounded.
- A short TCP port-22 reachability check across the local subnet found only the
  development computer. No SSH authentication to other hosts was attempted.
  This does not prove the Pi is powered off: its network or SSH availability may
  differ from its previous configuration. Expected historical Pi MAC is
  `b8:27:eb:ec:10:6a`; it was not present in the observed neighbor table.
- The ESP32 at `192.168.8.111` returned HTTP 200 for `/api/status`, identifying
  `1.10.24-history-read-test`, uptime `37827436` ms (about 10.5 hours), engine
  connected and polling, and free heap `67672` bytes. These were GET requests.
- `/pi` returned HTTP 200 with 19,119 bytes and a `/t1n-data.js` script reference.
  This verifies scanner-side page availability, not physical Pi display or touch.

## Existing evidence and candidate repair

The local archived source at `/home/ant/t1n-unified-monitor/pi` contains the former
launcher and Chromium supervisor. Its dated validation notes describe a previous
successful DSI-1 render and renderer recovery, followed by the rollback that
stopped the kiosk and disabled its autostart. Those notes are historical evidence;
today's deployed files, processes, services and touch mapping could not be read.

The archived launcher executes
`$HOME/.local/share/t1n-kiosk/esp-kiosk.py`. That supervisor uses a dedicated
Chromium profile and a process lock, a local offline HTML page, read-only scanner
health checks, and a `T1N_SCANNER_URL` environment override. It does not create a
measurement database or local proxy. Its default scanner URL still points to the
old `http://192.168.8.168/pi` address.

Private review artifacts are in
`/home/ant/t1n-batched-owner-build/pi-review/`:

- `observed.json`: the bounded reachability and scanner endpoint observations.
- `candidate-scanner-url.patch`: a one-line archived supervisor default change
  from `.168/pi` to `.111/pi`; it has **not** been applied to any Pi file.
- `README.md`: provenance, the existing environment override alternative, and
  verification requirements.

Prefer setting `T1N_SCANNER_URL=http://192.168.8.111/pi` in the existing launch
entry if the deployed supervisor is confirmed to match the archive. Do not
replace labwc autostart using guessed contents. The actual launcher, supervisor,
autostart, rc.xml, backup directories and legacy service/process state must first
be inspected. Preserve all unrelated autostart entries, DSI-1 touch mapping and
SSH access. No OS, driver, package or second backend installation is proposed.

## Remaining physical Pi checks

When the Pi is reachable, read its actual files and service state; compare with
the archived script; prepare the exact minimal URL/autostart correction and
preserve a local backup before applying it. Then verify one Chromium parent,
full-screen DSI rendering, existing touch interaction, read-only dashboard
requests, startup after boot and automatic recovery when the scanner returns.
Do not interrupt ongoing firmware checks to guess at an unreachable Pi's state.
