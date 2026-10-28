# ESP32 microSD migration — wiring, UI/assets, diagnostic history, partition layout

Status: wiring design verified against live 4-channel K-line firmware; **not connected, programmed, or installed**. Specific card-module voltage requirements still need confirmation from the physical module.

## Prepared stage-1 firmware and USB installer (2026-10-27)

- Firmware: `1.10.17-sd-stage1`; GitHub release contains `firmware.bin`, `bootloader.bin`, `partitions.bin`, `sdcard.zip` and SHA-256 manifest.
- The Home Assistant machine has matching verified release assets at `/home/ant/t1n-sd-stage1-release`, and an isolated esptool 4.9 environment at `/home/ant/.cache/t1n-esptool-env`.
- SD GPIOs in firmware: CS22, CLK18, MISO19, MOSI17. `/api/sd/status` provides card mount, capacity and per-page presence. `/api/sd/test` performs a temporary read/write/verify/delete test (scanner disconnected). `/api/sd/upload?name=index.html` and matching `advanced.html` and `manifest.json` accept bounded file uploads into `/www`. `/api/sd/file?name=...` permits checksum readback. The old embedded UI still works if no card.
- The SD asset bundle stores the *uncompressed full original pages*. `scripts/provision_sd.py --bundle /home/ant/t1n-sd-stage1-release/sdcard.zip` uploads all pages over the local network and checks the downloaded SHA-256 hashes; it refuses mismatched firmware versions.
- For wired install, first run `/home/ant/.cache/t1n-esptool-env/bin/python scripts/usb_flash_sd.py --port <verified-ESP32-port>` to make a private 4 MiB backup and inspect existing partitions. After confirming the backup and partition compatibility, add `--flash --release-dir /home/ant/t1n-sd-stage1-release` to install selectively. The script refuses NVS layout changes and does not mass erase. **Do not automatically assume the existing FT232 serial adapter belongs to the ESP32.**
- The new app binary is **1,353,504 bytes** and the new 1.875 MiB (1,966,080-byte) `app0` and `app1` partitions fit it. A wired USB partition update is required before OTA can accept this image.
- The raw backup includes old SPIFFS history and NVS settings. The old SPIFFS journals may not be readable from the smaller new internal SPIFFS partition; migration of the history to SD is **not completed in stage 1**. Keep the backup private and intact until the journals are recovered and the history views verified. The new SD card initially carries only dashboard assets.

## Before wiring or flashing

- Confirm the photographed module is powered at 3.3 V only.
- Use a FAT32 card and keep SPI wiring short; ensure the new wires do not short 12 V to GPIO or 3.3 V.
- Disconnect power while soldering.
- Do not connect the ESP32 to the van's OBD supply while using USB serial flashing unless the power and ground path is deliberately designed for it.
- Stage 1 retains all existing K-line pin assignments and does not add any new diagnostic ECU commands.

## Why this change
- Installed scanner 1.10.16-abs-km20 reports an OTA app cap of **1,310,720 bytes**, and history SPIFFS reports **1,318,001 bytes capacity** (60,491 bytes used on 2026-10-27).
- GitHub CI compiles with `esp32:esp32:esp32:PartitionScheme=min_spiffs` for **1,966,080-byte app slots** but Arduino's compile limit is **not proof that the running chip has that partition table**. OTA never safely changes the partition table.
- SD storage can host HTML/JS/CSS, fault definition catalogs, and the CRC-protected diagnostic history journals. It does **not** execute ESP32 application code, replace the OTA app slots or directly enlarge firmware flash.

## Existing K-line routing verified in the source and scanner
| Module | RX GPIO | TX GPIO |
| --- | ---: | ---: |
| Engine CR2, OBD7 | 13 | 23 |
| HZR/KI, OBD15 | 14 | 27 |
| EGS, OBD11 | 26 | 21 |
| ABS/EWM, OBD9 | 33 | 32 |

None of GPIO18, GPIO19, GPIO17 or GPIO22 is assigned in existing sketch code. Verify actual board breakout and any off-repository wires before connecting.

## Proposed microSD via SPI (pins subject to actual board verification)
| microSD module marking | ESP32 Dev Module |
| --- | --- |
| SCK / CLK | GPIO18 |
| MISO / DO | GPIO19 |
| MOSI / DI | GPIO17 / TX2 (GPIO23 belongs to engine K-line TX!) |
| CS / SS | GPIO22 |
| GND | Common GND |
| VCC | 3.3 V **only for 3.3V-rated modules**; other breakout boards may need 5 V supply to their onboard regulator and level shifting. Determine from photo/part number. |

ESP32 SD-SPI Arduino API is configurable: `SPI.begin(18,19,17,22); SD.begin(22,SPI,4000000);` (start at 4 MHz then increase if stable). Use regulated stable power, short SPI wires and decoupling; avoid GPIO0/2/5/12/15 bootstrapping conflicts and currently used UART pins.

## Phased change, rollback safe
1. **Bench hardware first.** Check VCC ratings, polarity, GPIO routes, continuity; test `SD.cardType()`, `SD.totalBytes()`, create+read+delete a small test file and reboot with/without card. Card FAT32. Do not put card SPI signals into any 12 V interface.
2. **Read-only UI move.** Prepare SD `/www/index.html`, `/www/advanced.html`, static JS/CSS/assets. Web server streams files from SD, with explicit content types, caching and missing-card error. Always retain an internal tiny recovery/status/OTA page if SD is removed/corrupt. Avoid copy of existing full HTML into PROGMEM in production builds. Keep boot and core K-line communication independent of SD.
3. **History migration.** Existing journals are three 32-byte/record CRC-checked SPIFFS ring files: `/hist_now.dat`, `/hist_15m.dat`, `/hist_day.dat`; backup their exact bytes before any partition change. Port file access to an abstract StorageFS (SD first, optional flash fallback). Preserve CRC, ring recovery and write backoff, and gracefully handle SD write failures without starving UART diagnostics. Keep current history exported until restored successfully.
4. **DTC catalogs and logs** migrate to SD. Version files and validate checksum to prevent stale mappings; keep ECU-specific decoder logic and safe command allowlists in firmware. Use bounded writes or batch logs in memory for flash/SD wear management.
5. **One-time wired serial flash to change partition layout**, after backing up NVS/Wi-Fi/config and SPIFFS history and verifying SD UI + SD log file. On the ~4 MB flash build, min_spiffs offers approx 1.9 MB per OTA app slot, preserving OTA. Partition changes **cannot be done by ordinary OTA** and may erase history/config. Test restore and Wi-Fi IP after reflash. Existing GitHub CI and installed table must finally match.
6. **Validation:** web UI fully functional, diagnostic polling on all four physical K-lines, historical charts persist across reboot, SD removal leaves OTA recovery available, and normal OTA update succeeds on the new partition table. Check real OTA-app partition size at runtime.

## Design priorities
- Never store essential boot or UART logic solely on SD.
- Never attempt ABS actuation/programming/clear-DTC operations while testing storage.
- No external logging/SD call should block time-sensitive K-line receive loops.
- Put a version and checksum in SD UI and catalog asset manifest. Card removal should not crash scanner.

## Final right-side wiring (board photo verified)

User has chosen six connections all on the ESP32's right-hand header, as viewed with USB socket at bottom:

| microSD pin | ESP32 right header label | GPIO |
|---|---|---:|
| 3V3 | 3V3 | 3.3 V rail |
| CS | D22 | 22 |
| MOSI | TX2 | 17 |
| CLK | D18 | 18 |
| MISO | D19 | 19 |
| GND | GND | Ground |

`TX2` is a usable GPIO17 even though it bears a UART alias; all four K-lines use independently assigned UART GPIOs (K_TX 23,27,21,32). No change to SD module voltage: 3.3 V only. This arrangement supersedes any previous MOSI25 references.
