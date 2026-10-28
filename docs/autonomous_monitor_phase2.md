# Autonomous SD monitor — stage 2 test firmware

**Branch:** `feature/autonomous-sd-monitor-20261028`  
**Firmware:** `1.10.20-autolog-test`  
**State:** Code implementation and CI compilation only until independently verified on hardware and with the van's ECUs. Do not treat a successful compile as completed on-vehicle qualification.

## Existing four K-lines

| ECU | OBD pin | RX/TX GPIO | Monitoring |
|---|---|---|---|
| CR2 engine | 7 | 13/23 | Main session, priority 120+ seconds |
| EGS transmission | 11 | 26/21 | 12-second window, every ~4 minutes |
| ABS | 9 | 33/32 | 8-second window, every ~10 minutes, **stationary only** with fresh speed/RPM |
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
3. `POST /api/monitor/dtc/selftest` — expect synthetic DTC comparison PASS without writing any real fault records.
4. `GET /api/monitor/clock` — verify against a known reliable clock, reboot and repeat with WAN unavailable. Confirm the unsynchronized flag is honest; compare elapsed uptime against elapsed epoch and inspect `jumps`.
4. With ignition on and van stationary, monitor `GET /api/monitor/status`; verify each module has at least one connection and `historyWrites > 0`, and validate values with `/api/history/data` and SD ring readback. Ensure the main engine recovers after each auxiliary window, and requests do not cause K-line timeouts.
5. For engine/EGS, verify complete DTC snapshots update last successful scan. Confirm no false new alert on identical repeat scans/restarts; verify NEW, STATUS_CHANGED and NOT_REPORTED with controlled fixtures before trusting alerts. For ABS/HVAC, verify `dtcSupported=false`, not "no codes."

Endpoints: `GET /api/monitor/status`, `GET /api/monitor/clock`, `POST /api/monitor/selftest` (disconnected only), `POST /api/monitor/enable?enabled=0|1`. To prevent automatic ECU requests on the bench, disable auto-monitor explicitly before putting the scanner into an extended test setup.

## Remaining work

- Restore or merge old history journals extracted from the original verified 4 MiB flash backup. Never overwrite new journal data without backup and reconciliation.
- Add battery-backed hardware RTC, ignition input / power management and safe shutdown testing.
- Verify real ECU sample rates and safe session timing before making a permanent release.
- Investigate ABS/HVAC DTC-read services from vehicle-specific captures and verified CBF mappings; do not guess protocol requests.
- Implement real alert thresholding and dedicated display after baseline logs are available.

## CI verification outcome — 2026-10-28

[GitHub Actions run 40742098933](https://github.com/antflix/t1n_obd/actions/runs/40742098933) **passed** on commit `a3367e31`:
- Native DTC transition tests: PASS (baseline, unchanged, reordered, status change, new, no-longer-reported, empty, event-buffer bounds).
- ESP32 Arduino 3.3.12 compilation: PASS.
- Program image: **1,365,992 bytes**, 69% of the 1,966,080-byte partition.
- Static global RAM: **102,160 bytes**, 31% of the 327,680-byte budget.
- Test artifact: `t1n-autolog-test`; upload succeeded. Moving production `latest` release was skipped.
- Full hardware validation is **NOT** complete because the Home Assistant Remote Desktop Commander device went offline. No ESP32 flash of this branch, SD self-test on the chip, four-module real history samples, ignition tests, or power-cycle clock accuracy tests have been performed.

The existing production ESP32 remains on `1.10.17-sd-stage1`. Stage 2 is deliberately held as a draft PR pending on-device verification. Clock reliability through a power loss without WAN still requires an external battery-backed RTC; a passing compile does not change that.

## Live ESP32 deployment validation — 2026-10-28

**Installed on scanner** at `192.168.8.168`: `1.10.20-autolog-test`, artifact from [CI run 40752610834](https://github.com/antflix/t1n_obd/actions/runs/40752610834), git commit `0c912935a44ddef665c07614a92b5105d34ad295`. SHA-256 verified before OTA. OTA responded 200 and device rebooted into the expected firmware. Production GitHub `latest` remains unchanged; original 4 MiB flash backup and prior stage1 image retained off-device.

**On-device smoke test: 14 PASS / 0 FAIL**, report `/home/ant/t1n-stage2-device-validation-20261028.json` on Home Assistant computer:

- Physical SD mounted and all dashboard assets remained present.
- `/api/history/status` reports microSD backend, 2,560,000 bytes of bounded CRC ring capacity, 160 active sensor slots, zero overflow.
- For module IDs CR2=0, EGS=2, ABS=1, HZR=3, the *actual journal routines* appended, wrapped, overwrote, CRC-validated and read back records on SD; 12 history writes completed with zero errors. These are deliberately temporary **synthetic** records, not real ECU measurements; the self-test files were removed.
- ESP32 in-memory DTC comparison self-test passed baseline, reordering, new, changed and no-longer-reported cases without affecting real DTC state; native CI DTC tests passed too.
- UTC epoch matched Home Assistant computer within 0 seconds during the on-device test. An additional 30-second, seven-sample test on `1.10.18` kept UTC within 1 second and detected zero jumps. This verifies network-synchronized clock operation, **not** timestamp recovery after power loss without a network (requires battery-backed RTC).
- Four history queries (CR2, EGS, ABS, HVAC) returned valid responses, **zero actual samples** because the scanner was not in an ECU session.

**Not yet verified/activated:** Live ECU sessions, actual samples and module coverage, session switching while engine running, persistence of real records across reboot, real engine/EGS DTC reads/changes, ABS/HVAC fault-memory services (still unsupported), ignition-aware power behavior. Autonomous mode is **OFF by default** on this first test rollout and must be explicitly enabled when the van is safely parked with ignition on. Do not claim continuous monitoring is operating until this test gate passes.

Old historical data preserved separately: **1,904 valid CRC journal records** recovered into private CSV `/home/ant/t1n-flash-backups/t1n-original-history-recovered-20261028.csv`, SHA-256 `fddf7671da81adc8b4f6d1d355782db9d2ab6648e0431bcfefb6b3b852f2e769`. Not yet merged with new SD history; original journal files and complete flash backup kept.
