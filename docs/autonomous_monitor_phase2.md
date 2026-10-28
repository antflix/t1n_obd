# Autonomous SD monitor — stage 2 test firmware

**Branch:** `feature/autonomous-sd-monitor-20261028`  
**Firmware:** `1.10.18-autolog-test`  
**State:** Code implementation and CI compilation only until independently verified on hardware and with the van's ECUs. Do not treat a successful compile as completed on-vehicle qualification.

## Existing four K-lines

| ECU | OBD pin | RX/TX GPIO | Monitoring |
|---|---|---|---|
| CR2 engine | 7 | 13/23 | Main session, priority 120+ seconds |
| EGS transmission | 11 | 26/21 | 12-second window, every ~4 minutes |
| ABS | 9 | 33/32 | 8-second window, every ~10 minutes |
| HZR HVAC | 15 | 14/27 | 8-second window, every ~15 minutes |

**Only one K-line session can be active:** all transceivers share UART2. The engine is temporarily disconnected during auxiliary module visits. These are scheduling targets; actual poll performance must be measured in the van. Unsuccessful ECU connections back off instead of generating DTC alerts.

Startup: monitor enabled by default (NVS toggle), attempts the engine after a 12-second boot delay, retries failed connection after 30 seconds. Requires ignition as appropriate; **no ignition-sense hardware is fitted**. Manual module connect/select/disconnect is never preempted by the scheduler, and produces a 5-minute scheduling hold. A manually connected ECU is left alone until disconnected.

## History and timestamps

- SD mounted **before** history service; uses `/t1nlogs/hist_now.dat`, `hist_15m.dat`, `hist_day.dat` (bounded CRC-checked 32-byte records, larger capacity on SD).
- If SD mounting fails, use pre-existing internal SPIFFS **without formatting it**. Failure is visible through the API. The SD card's existing files are never erased.
- `GET /api/history/status`: storage choice, clock status, write counters, usage and ring record counts.
- `GET /api/history/data?module=0..5&id=<metric>&range=24h`: history for all six module IDs.
- Numeric records require a valid UTC time. If the chip cold-boots with no NTP/browser clock, the logger will not invent absolute timestamps. `GET /api/monitor/clock` advertises `valid=false` until synchronized. It tracks apparent >5-second clock jumps using monotonic uptime.
- **An external battery-backed RTC is still required for reliable absolute timestamps through power loss without network time.** The current implementation must not be claimed to solve this physical limitation.

## DTC event tracking

- Only verified CR2 and EGS read-only service-18 paths are scanned (on connection and every 60 seconds while active). `readDTCs` must return a valid, complete packet before changing state.
- First complete scan establishes a baseline; subsequent snapshots compare by code regardless of order, recording NEW, STATUS_CHANGED, NOT_REPORTED in `/t1nlogs/dtc_events.csv`.
- Small last-successful snapshots persist in a separate NVS namespace on changes only to avoid repeated NEW alerts after reboot.
- Unsupported/failed ABS and HVAC DTC reads are **not** treated as zero faults. Their metric logging still proceeds. No automatic fault clearing or actuator commands.
- Event CSV includes UTC epoch (zero if unavailable), uptime milliseconds, module ID, action, code/status raw bytes.

## Device test sequence (required before merging or installing on the van)

1. Confirm `GET /api/status` reports the test firmware, `/api/sd/status` reports mounted, and `/api/history/status` reports `storage=microSD`; verify normal dashboard and retained pre-existing SD contents.
2. With *no ECU session active*, `POST /api/monitor/selftest` — expect **4/4** separate CRC files created, read, verified and deleted. This proves the SD paths, not actual module telemetry.
3. `GET /api/monitor/clock` — verify against a known reliable clock, reboot and repeat with WAN unavailable. Confirm the unsynchronized flag is honest; compare elapsed uptime against elapsed epoch and inspect `jumps`.
4. With ignition on and van stationary, monitor `GET /api/monitor/status`; verify each module has at least one connection and `historyWrites > 0`, and validate values with `/api/history/data` and SD ring readback. Ensure the main engine recovers after each auxiliary window, and requests do not cause K-line timeouts.
5. For engine/EGS, verify complete DTC snapshots update last successful scan. Confirm no false new alert on identical repeat scans/restarts; verify NEW, STATUS_CHANGED and NOT_REPORTED with controlled fixtures before trusting alerts. For ABS/HVAC, verify `dtcSupported=false`, not "no codes."

Endpoints: `GET /api/monitor/status`, `GET /api/monitor/clock`, `POST /api/monitor/selftest` (disconnected only), `POST /api/monitor/enable?enabled=0|1`. To prevent automatic ECU requests on the bench, disable auto-monitor explicitly before putting the scanner into an extended test setup.

## Remaining work

- Restore or merge old history journals extracted from the original verified 4 MiB flash backup. Never overwrite new journal data without backup and reconciliation.
- Add battery-backed hardware RTC, ignition input / power management and safe shutdown testing.
- Verify real ECU sample rates and safe session timing before making a permanent release.
- Investigate ABS/HVAC DTC-read services from vehicle-specific captures and verified CBF mappings; do not guess protocol requests.
- Implement real alert thresholding and dedicated display after baseline logs are available.
