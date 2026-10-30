# Firmware 1.10.25: SD history and CR2 reliability restore

- Standard browser WebUI and Pi charts both consume the device `/api/history/data` API, which reads recorded SD journal records. No browser-sample chart fallback remains. Live numeric gauges continue using `/api/sensors`.
- If SD is absent, stored history is unavailable and the UI reports that directly; the firmware does not silently switch to SPIFFS.
- Restored legacy vehicle-verified CR2 Autel replay fallback when short init fails.
- Added reset-reason and uptime status fields and a boot activity-log reset reason.
- Preserved SD upload of index.html, advanced.html, manifest.json, pi.html, and the /pi URL.
- SD history reads use sequential reads rather than per-record seeks in full circular journals.
- Normal WebUI polls every 2 s and only loads fault/raw group/log tabs when visible. Active tests explain the logger/manual connection requirement.

## Testing and deployment state

Compiled with ESP32 Arduino core 3.3.12, esp32:esp32:esp32:PartitionScheme=min_spiffs: 1,392,444 bytes / 1,966,080 bytes; memory 114,696 bytes / 327,680 bytes. Validated JavaScript syntax of both dashboards; verified SD ZIP checksums, /pi compiled into binary, and circular-log iterator equivalence. Native ignition/retry/DTC tests pass.

**Not yet deployed or vehicle-tested:** scanner was not accessible via network or USB serial at build completion. Do not flash while vehicle is moving or while operating an ECU diagnostic session. Preserve existing firmware backups/NVS.

## Final local verification (October 29, 2026)

- Embedded emergency WebUI now mirrors SD dashboard and does not synthesize browser history.
- Both dashboard scripts pass `node --check`; ignition/retry/DTC C++ unit tests pass.
- SD history API reports unavailable history with HTTP 503; the UI distinguishes no saved readings from a device or storage failure.
- Wi-Fi/server status is displayed separately from failed sensor requests.
- Local ESP32 Dev Module min_spiffs compile succeeded using Arduino core 3.3.12 (1,392,444-byte sketch; 1,966,080-byte max). Existing 1,920 KiB OTA partition matches the build.
- Firmware file: `firmware-1.10.25-sd-history-restore.bin`, SHA-256 `4938c402d4e61b69ae0ad07cce08b6d15e130bd09c558ebb4a9dcc58dc373af5`.
- ZIP bundle: `/home/ant/sprinter-firmware-download/esp32-sd-history-reliability-1.10.25.zip` (firmware and web assets).
- **Not yet installed.** ESP32 192.168.8.168 was unreachable by Wi-Fi, and no ESP32 USB serial device was connected during this session. Do not claim field reliability or perform an unattended flash.
