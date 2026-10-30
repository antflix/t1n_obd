# Current single-UART ownership and UI contract

This describes source `1.10.26-unified-sd-test`, not an installed-device claim.

Automatic monitoring owns the single reassigned UART whenever enabled, including retry/backoff periods with no connected ECU. Viewing pages, choosing chart metrics/ranges, reading stored faults, and changing display tabs issue no diagnostic commands. There is no five-minute page hold. An explicit monitor pause relinquishes its session; resuming releases a manual session and restarts the existing engine-priority scheduler.

All 25 modern/legacy diagnostic and transport configuration POST routes call the same automatic-owner guard. It returns HTTP 409 with `code=AUTOMATIC_UART_OWNER`. This includes legacy `/payload`, `/dtc/read`, `/dtc/clear`, relay tests, and shared-line probes, which previously bypassed the guard. Firmware/SD maintenance additionally requires no active session. No route is used by tests against the real van.

`GET /api/sensors` and `/sensors` serialize the same sensor catalogue through a fixed 256-byte JSON writer; no full sensor-response String is built. `sensorsJson(true)` visits the same definitions for logging without creating JSON. Each response identifies its module so a scheduler switch between status and sensors requests cannot assign EGS values to engine keys. Sensor age is monotonic; stale values are not rendered as current. Persisted record UTC is derived from sample receipt, with no fabricated wall time when synchronization is absent.

The shared client `firmware/web/monitor-client.js` is embedded into both layouts and the firmware fallback by `scripts/sync_web_assets.py`. It serializes requests, times them out, validates SD records, and segments chart gaps. It stores no measurement database. Mobile navigation and Pi navigation are read-only; alert rule changes/acknowledgments and monitoring mode controls are explicit user operations.

Scanner online, ECU connected, monitor enabled, data fresh, history readable, storage health and UTC validity are independent. History errors do not make a responding scanner offline. A missing/failed/unsupported DTC scan never becomes an empty successful fault set. Unconfigured vehicle alert thresholds never establish all-clear status.

The original remote 1.10.24 staging report is preserved in [autonomous_ui_staged_1.10.24_2026-10-29.md](autonomous_ui_staged_1.10.24_2026-10-29.md); its historical installation/network statements are superseded by current read-only observations.
