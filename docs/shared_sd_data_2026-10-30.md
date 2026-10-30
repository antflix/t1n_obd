# Shared SD data — 2026-10-30

Candidate **1.10.19-shared-sd-test** starts from the working **1.10.18-autolog-test** source (`8f8c7b8`). This is a focused data-flow replacement, not the later 1.10.28 development bundle. The replacement ESP32 was observed running 1.10.18 at `192.168.8.111`, with a connected engine session, live polling, mounted SD, and autonomous monitoring disabled, before this work.

## One data flow

ECU samples enter one canonical sensor visitor in firmware. The SD logger observes that visitor independently of browsers; the live API streams the same sensor catalogue without creating historical observations. Live readings remain validated RAM measurements. Charts and CSV/SVG exports consume only persisted SD records, with a bounded browser cache of server responses. The same client, freshness/pairing rules, chart renderer and exporter serve mobile and Pi.

Removed the browser sample collector, fabricated browser timestamps, browser live-chart mode, alternate browser datasets, silent browser fallback, duplicate chart renderer, and browser-generated history export. Removed internal SPIFFS history fallback and the old module-switch accumulator reset. No migration reader, alternate history database, new Pi backend, or SD format operation was added. The existing compact journal layout is adequate and remains the sole layout; retaining it does not introduce a second format or migration path. Existing data was not promised compatible or needed for validation.

## Contract and bounds

- `GET /api/history/status`: `schemaVersion:1`, `available`, `storage:"microSD"`, clock, recording policy, capacities, successful writes, read/write errors, corruption and overflow counts, observed maximum SD operation durations.
- `GET /api/history/data?module=0&id=rpm&range=24h`: final response has `ok`, `schemaVersion:1`, `storage`, `module`, `id`, `range`, `points`, `count`, `truncated`, `clockValid`, `corruptRecordsSkipped`, and `recording`. Each point has original UTC seconds `t`, `min`, `avg`, `max`, `count`, and numeric `tier` (0 recent, 1 quarter-hour, 2 daily).
- Accepted ranges: `1h`, `24h`, `7d`, `30d`, `all`. Relative ranges require valid UTC; `all` can display valid saved timestamps even before the current clock synchronizes.
- One cooperative SD query runs at a time, targeting 128 records or 2 ms per loop slice. HTTP 202 `HISTORY_LOADING` asks the client to retry; another browser's competing query returns bounded `HISTORY_QUERY_BUSY`. Completed responses wait until diagnostic transactions are idle. Abandoned queries expire. Up to the newest **200 records** are returned chronologically; truncation is explicit.
- Clients serialize actual HTTP requests, while history loading waits allow live requests to run. Cache: at most 16 server datasets, 15-second lifetime; no stale-cache fallback after errors. Charts support eight series with separate unit panels, min/max envelopes and discontinuities across major gaps.
- Recording remains adaptive: changed records at a minimum ten-second spacing, unchanged heartbeat at thirty minutes, plus quarter-hour and daily summaries. Charts reflect saved resolution; they are not full-rate traces. Individual ECU sample timestamps are deduplicated before aggregation.
- Integrity is **FNV-1a-32**, accurately named rather than claiming the existing journal checksum is a CRC.

## UI and deployment

`firmware/sdcard/t1n-data.js` is the shared source; `scripts/package_web.py` generates its embedded firmware asset and a matching SD zip. The compiled shared-client endpoint also permits the embedded mobile interface to display live readings and explicit storage errors without a usable SD card. The SD zip contains `www/index.html`, `advanced.html`, `pi.html`, and a checksummed manifest. SD pages are served only when their manifest version matches the running firmware, preventing old browser-history HTML from remaining active after the firmware replacement. `/pi` reports missing dashboard assets and retries if the matching Pi page is absent.

Opening or navigating either interface performs no POST. Viewing a module is local display state; the obsolete state-changing `/api/module/select` route is removed. Explicit manual diagnostic controls remain. Browser clock synchronization is an explicit button, never an automatic page-load action.

ECU transport, physical pin routing (engine RX13/TX23; SD CS22/CLK18/MISO19/MOSI17), initialization, shared-line protocol handling, DTC commands, saved diagnostic configuration, and scheduler behavior are unchanged. The only monitor-file change removes the deleted fallback flag from its existing storage self-test gate.

## Verification

- `python3 scripts/checks.py`: native history fault/recovery/range/rollover tests, existing DTC comparison tests, >8 KB bounded JSON parsing/escaping, shared client, mobile/Pi fixtures, package/source contracts.
- `NODE_PATH=/tmp/t1n-browser-tests/node_modules node tests/web_dashboards.cjs`: Chromium and WebKit; same SD rows after reload, ECU-disconnected history, independent chart module during live module changes, loading/empty/error/truncated states, SD failure with live readings, sensor API failure with scanner still online, stale/offline/recovery, 375 px mobile and 800x480 Pi layouts, and zero navigation POSTs.
- History native tests additionally passed AddressSanitizer and UndefinedBehaviorSanitizer.
- Initial Arduino build with ESP32 core 3.3.12 and `PartitionScheme=min_spiffs`: 1,400,320 bytes program, 115,040 bytes static RAM. Final artifact size/checksum and deployment evidence are recorded separately after final packaging.

The working firmware remains the rollback option. Credentials and raw backups are private and excluded from Git. Public development CI builds omit the Wi-Fi build secret; the private installation artifact retains the existing network configuration. No raw vehicle recordings are committed.

## Remaining validation and limits

Initial hardware validation passed after installation; extended reliability testing remains pending. A 2 ms scan budget cannot bound an individual SD driver's read or flush latency; measure the exposed maximum durations on hardware. The single-thread firmware still services some HTTP requests inside existing transport waits; this phase does not claim full task isolation or change the proven transport implementation. The inherited manual-monitor ownership/retry architecture is not reworked in this phase, and unqualified autonomous ABS visits must remain disabled. This candidate does not implement new alert rules or change the Pi's deployed startup/services.

## Installed verification

Installed `1.10.19-shared-sd-test` over OTA at `192.168.8.111` after the user explicitly requested upload. Application SHA-256: `69a07af9e796c6bcc2ccc36ab1335ec6dfc8cbc276f7f0f071635a340fb1e3bb`. The firmware reported the new version after reboot. Standard curl multipart transfers were interrupted; a paced Python multipart transfer completed with HTTP 200. The cause of the interrupted transfers is not established. No USB flashing or new flash backup was needed.

Installed all four matching SD dashboard assets, verified each by SHA-256 readback, and verified that `/` serves the packaged index. `/pi` and the compiled `/t1n-data.js` both returned HTTP 200. Restored the previous manual engine session; autonomous monitoring remains disabled as it was before installation. Engine quick initialization succeeded and all 49 sensor entries were valid and fresh. The sensor JSON response exceeded 8 KB and parsed successfully.

A real SD-backed one-hour RPM query returned 174 records through the cooperative loading contract in approximately 5.6 seconds while engine polling continued. Initial post-query health reported 56 successful writes, zero read/write/corruption errors, maximum observed read 15 ms and write 17 ms. Existing journals and unrelated SD files were not erased. Subsequent vehicle-speed readings indicated movement, so all remaining checks were strictly read-only; no additional session changes, resets, or firmware updates were attempted. Private detailed responses are kept with the installation artifact rather than committed as vehicle recordings.

This verifies installation, initial engine connectivity, streamed JSON, asset serving, and actual SD reads/writes. It does not establish long-duration reliability, qualification of secondary modules, or Pi kiosk startup behavior. The known-good 1.10.18 artifact remains available for rollback.

## Chart usability repair — 1.10.20-chart-ui-test

The user identified the message as “measurements are not time-aligned,” rather than a history-query timeout. This is the live paired-reading difference guard: actual and requested values are refreshed by different ECU requests. The strict two-second alignment gate remains; the UI now explicitly identifies the unavailable **live difference**, reports sample separation, and explains that saved charts remain available. A read-only hardware query returned 200 saved rail records in about 7.2 seconds while engine polling continued, with 5,647 successful SD writes and no recorded read, write, or corruption errors at the pre-change check.

The history area now shows an immediate loading placeholder, completed-series count, automatic-completion guidance, and accessible busy state. Repeated taps on the already selected metric retain the in-progress query and current comparison selection instead of resetting chart state. No browser history collector, timing bypass, transport change, scheduler change, or new history fallback was added.

The measurement selector now has a visible Done button outside its scrolling list, a Hide metrics toggle with expanded state, integrated search, and selected-series count. Closing preserves choices. Paired cards occupy the correct parent grid columns and numeric values no longer split mid-number on narrow screens.

Verification: all offline shared SD checks passed. Chromium and WebKit fixtures passed, including six delayed history-loading responses with four-second live sample skew, repeated taps preserving the active load, automatic chart display despite unavailable live differences, selector Done/filter/long-list behavior, preserved selections, and four-digit value layout at 375 px. Installation results are recorded after deployment.

Installed `1.10.20-chart-ui-test` after the user explicitly instructed proceeding while the vehicle was moving, superseding the earlier parked-only instruction for this display/scanner update. The paced OTA transfer returned HTTP 200, reboot reported the new version, and all four SD assets passed SHA-256 readback. No ECU actuation, fault clearing, or coding commands were issued. Restored the existing read-only engine session; quick initialization succeeded. Artifact: 1,402,528 bytes, SHA-256 `8b92a1817b58609e0968f8b4e2d562287ea6a7ef43ef517ae8727971f688e814`, within the 1,966,080-byte OTA partition. Source commit `9f8e6bda2d0983265b07e7cb7c63611b84b8b565`.

A real Chromium mobile viewport against the installed ESP32 loaded 200 rail-actual and 200 rail-target SD records automatically, preserved the in-progress load across repeated taps, and verified that Done closes the selector. That browser sent zero POST requests and raised zero JavaScript errors. Final read-only checks showed the engine connected and polling, 49 fresh sensor entries in valid JSON, 88 newly persisted records after reboot, zero read/write/corruption errors, and maximum observed read/write durations of 14/28 ms. Private responses and screenshot remain with the local release artifact. These are initial hardware checks, not an extended reliability qualification.
