# Initial SD logging rollback — 2026-10-30 UTC

At the user's explicit request, stopped the later development and restored the first SD measurement-logging build, **1.10.18-autolog-test**, source commit `8f8c7b83506abf9e927fcc7136c26b38ad547f39`. The preceding 1.10.17 stage added SD web assets but did not yet store measurements on SD. This branch preserves the original 1.10.18 firmware source; this note is the only added file.

## Installed and verified

- Used the preserved original application, not a newly compiled variant: **1,367,184 bytes**, SHA-256 `cedbef396868e68bd8f3f80a3b329bc8ef79470029c6cc58d54d2781915298fd`.
- Wrote only the selected application at `0x10000`, using the identified CP2102 and 115200 baud. A separate digest verification matched the installed image. Bootloader, partition table, OTA metadata, complete other application slot, SPIFFS and historical crash partition matched the pre-work backup.
- Running `/api/status` confirms 1.10.18 at `192.168.8.168`. Transport configuration matches the saved configuration from the previously connected 1.10.24 installation exactly, including the 1000 ms pause after completed diagnostic requests. Wi-Fi credentials were preserved.
- Restored the original matching SD `index.html`, `advanced.html` and manifest, with checksum readback and verification of `/`. Previous pages were backed up privately before replacement. SD temporary write/read/verify/delete passed. No format, journal conversion or old-record merge was performed.
- Existing SD journals remain in place: 16,752 recent, 5,350 quarter-hour and 284 daily records at rollback verification, zero reported write errors. A saved RPM history query was checked separately. The full flash backup is **not** a raw microSD backup; no claim of such a backup is made.
- Stopped the new Pi kiosk supervisor/browser and commented only its launch line in labwc autostart. Original autostart was backed up. No kiosk processes remain. SSH, desktop, touchscreen mapping and drivers are preserved. The unused Pi HTML file was left on SD; firmware 1.10.18 does not expose `/pi`.

## Connection result and final operating state

The user confirmed the OBD harness is attached, key ON and engine stopped, and later reseated the connector. A read-only engine connection attempt on the restored 1.10.18 still received only the scanner's transmitted bytes echoed back, with no ECU StartCommunication reply. **Rollback is complete; restored engine communication and fresh engine logging are not verified.** Automatic module visits remain OFF and no diagnostic session is active. This also avoids enabling the older build's unqualified unattended ABS visits.

Before this rollback, the byte-verified original 1.10.24 image failed the engine handshake under the same saved settings, while its verified EGS connection succeeded and returned real replies. That comparison does not establish the cause of the engine-specific failure. No actuator test, DTC clear, ECU reset/coding or undocumented diagnostic command was used.

The user reported a router fault and rebooted it; scanner Wi-Fi access returned. Earlier private network-only probes and a later initialization-worker prototype are absent from this rollback. The worker prototype was never deployed. A proposed PHY-calibration reset was canceled before execution; no calibration namespace was erased. Later development remains preserved on its separate branch and in private backups. No further experimental deployment is pending.

Private evidence and backups are retained under `/home/ant/t1n-serial-20261030/`, with the original release at `/home/ant/t1n-stage2-isolated-20261028/`. No credentials, raw flash images or private vehicle logs are included in this branch.
