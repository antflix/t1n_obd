# History query latency — 2026-10-30

The user confirmed that the previous scanner-offline problem was resolved, but
saved charts still loaded slowly. Read-only requests against installed
`1.10.23-narrow-http-test`, with the engine session polling, measured:

| One-hour series | Total query time | Requests | Completed response | Records |
| --- | ---: | ---: | ---: | ---: |
| Rail actual | 7.224 s | 27 | 70 ms / 16,259 bytes | 200 |
| Rail target | 7.151 s | 27 | 71 ms / 16,343 bytes | 200 |

The two series are intentionally serialized, so the pair required approximately
14.4 seconds before browser rendering. The recent journal was full at 32,000
records, with 5,945 quarter-hour and 333 daily records. The query scanned every
journal using one 32-byte filesystem read per record. Rendering was not involved
in the measured wait.

## Narrow replacement in 1.10.24-history-read-test

Replace individual record reads with up to sixteen records per filesystem read,
using one static 512-byte buffer. Preserve the existing 128-record maximum and
two-millisecond cooperative slice target; check elapsed time after each batch.
An individual SD operation can exceed that target, as before. There is no new
history index, database, persistent format, browser recorder or fallback.

Only scan recent records for one-hour requests, recent and quarter-hour records
for 24-hour requests, and all tiers for longer requests. Existing per-record
window checks, checksum validation, bounded newest-200 selection, chronological
output and HTTP sender remain in use. Diagnostic transport, scheduler, logging
and browser code are unchanged. Static RAM increases by 512 bytes.

Native production-code tests verify 2,000 reads for a full recent journal rather
than 32,000, at most eight reads/128 records in one slice, unchanged transaction
ownership guards, partial final batches, rollover, corrupt records in later
batches, short-read errors and exclusion of irrelevant unreadable tiers. Existing
range, persistence, JSON, shared-client and display regressions also pass.

The earlier one-hour observer stopped after approximately eleven minutes and
did not produce a completion summary; no completed one-hour test is claimed.
A subsequent status request on the unchanged firmware showed continuous uptime
of approximately 36 minutes and an active engine session. Hardware timing and
installation evidence for the new candidate will be appended after verification.

## Installation and measured result

Installed source `eb90f0d575e2862e923fbc409f6ed5143bdcae0f`, application
1,406,016 bytes, SHA-256
`aa1b4b27fa9a55581861f47da79127d9445b7e9c37884658f99cac1796357ced`.
The image fits the 1,966,080-byte OTA partition. The build reports 115,568 bytes
of static RAM; the history regression also passes ASan/UBSan. Matching SD assets
passed SHA-256 readback, and engine quick initialization/polling was restored.

Three active-session rail actual/target pairs took **9.454, 10.292 and 9.796
seconds**, versus 14.375 seconds before. Individual series took 4.700–5.250
seconds. Completed response transfers remained 68–79 ms. This is approximately
one-third faster, not instant loading. The full recent journal is still scanned
for each metric; no ordered-timestamp assumption or second index was introduced.
The post-benchmark snapshot showed 58 successful new SD writes, zero storage
errors, maximum read slice 14 ms, minimum stack 1,360 bytes and free heap 68,304
bytes. No HTTP timeout/failure had occurred during the benchmark itself.

**The subsequent stalled-history regression did not pass.** Its final sensor
recovery request timed out; subsequent status requests and ICMP also failed.
HTTP then recovered once with uptime 179,001 ms and `resetReasonCode:9`:
`ESP_RST_BROWNOUT`, verified against the installed SDK's `esp_system.h` enum.
The reset left diagnostics disconnected; subsequent HTTP requests timed out
again. This is evidence of a detected supply brownout, not proof of its physical
cause or a completed stability verification. A user question about power changes
is pending. Do not disable brownout detection or declare the new build qualified.

The live dual-browser harness initially could not import Playwright because
`NODE_PATH` was unset; it made no browser requests and is not counted as a pass.
Real display/stall rechecks and restoration of the engine session remain pending
device availability. Existing offline display tests passed. No further runtime
code changes were made in response to this interruption.

## Recheck — 2026-10-31

The user clarified that the brownout was unrelated to the scanner work. Treat
the earlier interruption separately rather than attributing it to the reader.
No further firmware changes or restarts were performed for this recheck.

The unchanged installed `1.10.24-history-read-test` was reachable with 77 minutes
of uptime, the engine connected/polling, and 1,568 successful history writes.
SD read/write/corruption and HTTP error counters were all zero at that snapshot.
This is a current uptime/status observation, not a completed continuous one-hour
browser observation. Scheduled autonomous multi-module monitoring remains
disabled; the existing engine session continues logging independently of pages.

The previously interrupted stalled-history test now passes: a deliberately
unread completed history response times out within the bounded sender, a
concurrent status request returns in 15 ms, uptime continues, and sensor output
recovers with 49 items. Exactly one intentional HTTP timeout is recorded.

With the existing Playwright installation supplied via `NODE_PATH`, both actual
mobile and Pi pages loaded rail actual/target charts (119 and 103 saved records
respectively on both interfaces). They remained online with the engine connected
through the subsequent 30-second dual-display observation, sending no POSTs and
producing no JavaScript errors. Final snapshot: uptime 4,724,303 ms, 1,591 successful
SD writes, zero SD errors/corruption, no HTTP failures, and the single intentional
HTTP timeout. These targeted hardware checks pass; longer combined workload
qualification and autonomous scheduling work remain separate.

The next performance target is the duplicated journal search for separately
requested metrics, especially actual/target pairs. Measure memory and read-slice
costs before replacing that path; a browser renderer/library cannot remove it.
