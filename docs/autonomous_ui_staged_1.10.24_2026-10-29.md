# Single-owner autonomous logger / WebUI update (2026-10-29)

**Source branch:** `feature/autonomous-sd-monitor-20261028`
**Built, CI verified, and staged for safe OTA:** `1.10.24-auto-ui-test`
**GitHub workflow:** https://github.com/antflix/t1n_obd/actions/runs/40826579787 — SUCCESS, native unit tests and Arduino ESP32 compile
**Firmware SHA-256:** `4ff7f4fd77009b2785c6387205c3dea3d7f431bb3d530351f69f680978cf40d4`, 1,374,880 bytes
**Build commit:** `16c1117894ba1a5883555e1e99b2fbe82a21aa0f`
**Home Assistant staged binaries:** `/home/ant/t1n-1.10.24-auto-owner-staged/firmware.bin`, `index.html`, `manifest.json`, `sdcard.zip`. Verified artifact manifest and full 4 MiB original flash backup.

## Behavior change

1. Removed five-minute manual diagnostic override (`monHoldUntil`, `monitorManualHold`, `monitorManualOverride`) from the ESP32 firmware. Opening the WebUI never changes the logger state.
2. The automatic logger owns the shared K-line UART whenever its persisted NVS flag `enabled=true`. Enabling it releases any existing manual diagnostic session (only when idle) and immediately queues engine monitoring; disabling it releases a logger-owned session. No arbitrary timer.
3. Manual connect/select/disconnect, raw reads, active tests, actuator commands, live poll start/stop, and transport configuration are explicitly rejected with HTTP 409 while automatic monitoring owns the UART. To perform manual diagnostics, explicitly pause automatic logging first. Read-only requests, charts, logs, DTC display, and monitoring status remain available.
4. The native WebUI on ESP32 SD root `/www/index.html` was updated in source: Connect/Disconnect buttons removed; displays real `/api/status` connected/polling together with `/api/monitor/status` enabled/standby, with one explicit auto enable/pause control. Module list is read-only while autoscheduling. No browser action claims connection success.
5. Fixed malformed sensor JSON after ~8KB. The old `/api/sensors` buffer showed dangling separators `},,` and `},,,,,` when ESP32 String allocations failed at highway monitoring; new implementation streams bounded ~2KB chunks, preserving the complete full metrics schema and history capture behavior. Pi and phone can use same API without client changes.

## Installation status and safety gating

**NOT YET INSTALLED** because live engine reported road speeds 111–113 km/h during work. Original firmware `1.10.22-consecutive-retry-test` remains running, currently `enabled=true`, engine connected and polling, SD write errors 0, engine sample counter reached 2645. Do NOT cut diagnostic sessions or upload firmware/SD assets while the van is moving.

When the vehicle is confirmed parked, verify actual ECU speed fresh 0 km/h (or explicit ignition off), and verify OTA file SHA and device current version. Pause automatic monitor using POST `/api/monitor/enable enabled=0` (new auto disable change is not yet installed), stop polling and disconnect the old manual/session if needed (retry 409 busy after small delay), then upload the new OTA firmware with multipart `firmware` at <=48kB/s as previously successful. Verify rebooted /api/status firmware exact `1.10.24-auto-ui-test`. Upload SD assets with POST `/api/sd/upload` multipart field `file` and form field `name=index.html` (and `manifest.json`), while scanner disconnected, preserve SD history. GET /api/sd/status and /api/sd/file?name=index.html to verify, then POST /api/monitor/enable enabled=1, confirm background engine auto-connect, real history writes rising, zero SD errors, web UI shows real auto status. Retain original SD assets on HA before overwrite.

## Pi touchscreen

Home Assistant PC `192.168.8.250` successfully SSHes as `ant` with authorized key into Pi 3B `192.168.8.136`. Pi runs Raspbian GNU/Linux 13 Trixie and local dashboard server `127.0.0.1:8765`. `/dev/input/event3` is `10-0038 generic ft5x06 (79)` recognized as touchscreen by libinput. DSI-1 connected and HDMI disconnected. Labwc config already maps the touchscreen to DSI-1: `<touch deviceName="10-0038 generic ft5x06 (79)" mapToOutput="DSI-1" mouseEmulation="yes"/>`. Further physical touch-event test needed when the user is parked at the screen; do not reconfigure while moving.
