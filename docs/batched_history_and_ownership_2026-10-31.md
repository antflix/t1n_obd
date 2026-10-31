# Batched history and explicit UART ownership — 2026-10-31

Baseline is installed `1.10.24-history-read-test`. Before this change, a GET-only
check observed uptime 37,591,411 ms, engine connected/polling, 13,210 successful
SD writes and zero SD errors. Its saved poll interval is 300 ms after a completed
transaction; this work preserves that value and all initialization/wiring.

## One history query contract

`GET /api/history/data?module=0&ids=rail,railTarget&range=1h`

The old `id` request and single-series response are removed. The sole successful
response has `schemaVersion:2`, `ok:true`, `storage:"microSD"`, `module`, `range`,
`clockValid`, `corruptRecordsSkipped`, `recording`, and ordered `series` entries:

```
{id, points:[{t,min,avg,max,count,tier}], count, truncated}
```

Requests accept one or two distinct IDs of up to 50 letters, digits, underscores
or hyphens. Invalid selections return HTTP 400 `INVALID_SENSOR_SELECTION`.
Loading and competing-query responses retain HTTP 202 `HISTORY_LOADING` and
HTTP 503 `HISTORY_QUERY_BUSY`, respectively. Errors contain `series:[]` and
never substitute samples. History status remains its separate schema version 1.

Each pair shares one SD scan and retains at most 200 records per metric. Fixed
result memory is 14,400 bytes, plus the existing 512-byte SD buffer. The two-pass
bounded JSON sender remains unchanged: no response-sized allocation, a 1,436-byte
encoder buffer, and one 250 ms socket-output budget. Native tests cover a paired
payload exceeding 40 KB, exact Content-Length and a stall partway through it.

The reader keeps the existing 128-record maximum and two-millisecond slice
target; individual SD calls may exceed that target. One-hour requests scan only
recent records; 24-hour requests add quarter-hour records; longer ranges add
daily records. FNV-1a integrity checks, out-of-order timestamps, independent
truncation, original UTC samples, missing intervals and journal layout remain.

Both displays call the same shared client with up to eight selected IDs. It
partitions them into serialized pairs and validates every response. The bounded
15-second cache contains server results only. Obsolete queued selections are
skipped before starting SD searches; current consumers of an identical pending
pair still share it. A search already started is consumed so it does not leave
the server's single query slot abandoned. There is no browser measurement
database, legacy response decoder, fallback data source or Pi backend.

## Explicit operating mode

Automatic monitoring owns the UART whenever enabled, including disconnected
backoff. The five-minute hold and indefinite yield to manual sessions are
removed. Pausing explicitly releases the automatic session; resuming releases a
manual session and schedules engine acquisition. Repeated mode requests preserve
the running session. The enabled preference still persists across boots.

All 24 manual UART entry routes, including old aliases, now return HTTP 409
`AUTO_MONITOR_OWNS_UART` until monitoring is paused. In manual mode, busy or queued
connection work returns HTTP 409 `DIAGNOSTIC_BUSY`. Existing flags are held across
disconnect and connection verification because transport waits can serve nested
HTTP requests. Browsing pages, historical queries and module selection for
display remain read-only. Firmware updates remain separate authorized operations.

Unattended ABS visits are removed and a qualification gate also excludes ABS at
the automatic connection entry point. Existing engine/EGS/HZR scheduling and
retry delays are retained; this does not qualify secondary hardware or change
K-line transport. Detailed evidence and remaining scheduler limits are in
`autonomous_baseline_audit_2026-10-31.md`.

Advanced provides explicit pause/resume controls with inline errors. Merely
opening the page sends no mode changes. The normal mobile page identifies
automatic ownership separately from the diagnostic connection; Pi remains a
read-only display.

## Verification and release state

Native tests compare batched results with separate searches across every range,
including corruption, read failure, rollover, independent truncation and invalid
time. A full 32,000-record recent journal requires 2,000 sector reads for a pair
instead of 4,000 for separate queries. The normal 400-point fixture is 28,427
bytes; this is a host fixture, not an on-device speed claim.

The actual production autonomous scheduler is compiled into a native harness
covering startup, persistence, pause/resume, nested HTTP exclusion, engine/EGS
visits, existing retry timing and ABS exclusion. Native policy/history tests also
pass ASan/UBSan. Shared-client, mobile/Pi and real Chromium/WebKit fixture tests
cover reloads, missing storage/data, offline recovery and read-only navigation.

Candidate `1.10.25-batched-owner-test` is being integrated. Final build checks,
artifact checksum, device installation, actual paired timings, healthy/stalled
response checks and explicit mode transitions will be recorded after completion.
No active tests, DTC clearing, ECU coding, SD formatting or private backup upload
are part of qualification. The installed 1.10.24 image remains the rollback.

## Hardware qualification — not passed

Installed source `80547bed878184ed8a00daf866c578261592690c` as
`1.10.25-batched-owner-test`, image 1,412,992 bytes, SHA-256
`35aa9798d49afd8269ddb76b27d31e29a0b74b29b5622848fb679f99cd5f0dce`.
The final build uses 122,864 bytes of static RAM, 7,296 bytes more than version24.
Matching dashboard files passed SHA-256 readback before the OTA update.

The enabled preference was saved before the update. After reboot, version25
reported automatic mode enabled while initially disconnected, then acquired the
engine and started polling without a browser Connect request. Monitor status
reported automatic ownership, one successful engine connection and ABS excluded
from unattended operation. A deliberately invalid manual connection request
(`id=-1`, safe even if the guard were missing) returned HTTP409
`AUTO_MONITOR_OWNS_UART`.

The next live test requested `ids=rail,railTarget&range=1h`. Its response headers
arrived, but reading the body timed out after five seconds. No complete paired
query was obtained. The test failed before reaching pause/resume or largest-pair
checks. Later HTTP health requests and ICMP did not respond, while the router
remained reachable. The HTTP status/length of the failed response were not
persisted before body reading; it is unknown whether it was a pending202 or a
completed200. No new reset reason is available. Do not attribute this interruption
to the earlier unrelated brownout or declare the candidate stable.

The immediately preceding version24 test retrieved the same two metrics
separately in 9.318 seconds (48 actual and146 target records). This was not a
400-point maximum-payload test. The last captured version25 startup status had
free heap91,332 bytes, minimum85,596, largest allocation49,140 and minimum observed
stack4,876; those are startup measurements, not measurements at failure.

An independent offline review found no buffer overlap or out-of-bounds selection
in the ELF/native tests. Worst-case host serialization produced42,794 bytes
through the actual firmware serializer. That is not proof of Wi-Fi throughput.
Compiled `historyDataSend` reserves3,232 bytes of stack versus3,088 in version24,
including on its pending response path. Automatic DTC nesting may add about416
bytes compared with the regular manual-poll path, for roughly560 additional bytes
combined. These figures identify a stack-pressure hypothesis, not a confirmed
overflow. Pending202 responses still use the inherited blocking WebServer sender;
that may block HTTP but ordinarily does not explain loss of ICMP.

No further runtime fixes were layered onto the unqualified image. The private
release directory contains a checksum-verified `restore24.py`, which pauses
automatic operation, reinstalls the retained version24 pages/image, verifies the
version and restores the former manual engine session. It has not been executed
because the scanner is unreachable. A physical power/reset clarification is
pending. No unrelated USB serial device was opened or reset.

Next: regain scanner access and restore the working baseline. Then capture every
response status/Content-Length before reading its body, and isolate one-metric
versus paired requests, followed by paused versus active diagnostics, preserving
SD records and transport timing. Actual paired speed, mode-transition/reboot
qualification beyond initial startup, secondary-module visits and physical Pi
kiosk verification remain incomplete.
