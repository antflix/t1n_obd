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

Candidate hardware validation is pending. A 2 ms scan budget cannot bound an individual SD driver's read or flush latency; measure the exposed maximum durations on hardware. The single-thread firmware still services some HTTP requests inside existing transport waits; this phase does not claim full task isolation or change the proven transport implementation. The inherited manual-monitor ownership/retry architecture is not reworked in this phase, and unqualified autonomous ABS visits must remain disabled. This candidate does not implement new alert rules or change the Pi's deployed startup/services.
