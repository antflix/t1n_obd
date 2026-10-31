# History stack isolation and recovery — 2026-10-31

After the user restarted the scanner, version25 responded with automatic engine
monitoring active. The power-on reset reason describes that restart and does not
identify the cause of the preceding outage. Automatic mode was explicitly paused
before recovery; no active tests or fault clearing were performed.

## Verified rollback

An initial page upload and paced firmware transfers were reset by the peer.
During these failures version25 stayed online with continuous uptime, monitoring
paused and Wi-Fi RSSI approximately -50 dBm. No explanatory HTTP error body was
returned. A continuous transfer of the exact retained version24 image succeeded
with HTTP200 `Firmware installed. Rebooting...`. This demonstrates a transfer
method dependency, not its underlying cause. No firmware upload parser was
patched and no unrelated USB device was opened or reset.

Verified installed `1.10.24-history-read-test`, restored its four matching SD
assets with SHA-256 readback and resumed the former manual engine session. All
49 sensor entries parsed. A saved one-hour rail query returned36 records in
4.891 seconds; eight new SD writes occurred during that query. The final check
showed131 successful writes and zero read/write errors. Automatic mode remained
disabled, as in the original working baseline. Existing history, SD contents,
NVS configuration and the saved 300 ms inter-request pause were preserved.

The private rollback helper now uses the successful continuous transfer and
restores firmware before its SD assets. It handles an automatic or manual session
explicitly and verifies the installed version before restoring engine polling.

## Isolated candidate

`1.10.26-history-stack-test` differs from version25 in one runtime instruction:
`writeHttpJson` is marked `__attribute__((noinline))`. Version metadata is also
updated. The history API, paired selection, SD reader/writer, UART ownership,
transport, scheduler, packet buffer size and250 ms output deadline are unchanged.
There is no extra history store, decoder, retry loop or fallback.

The preceding private experiment proved that preventing inlining removes encoder
storage from loading/error paths: `historyDataSend` shrinks from3,232 to544 bytes,
and the completed handler plus separate writer uses2,048 rather than3,232 bytes.
That is a measured resource improvement, not proof that stack exhaustion caused
the failed version25 test. Final version26 frame measurements are recorded with
its build artifacts before deployment.

The controlled device test starts with monitoring paused and diagnostics
disconnected. Test one metric, then its actual/target pair. Only after that passes,
connect the engine and compare active queries, followed by automatic ownership
and explicit mode transitions. Persist HTTP status and Content-Length before
reading bodies so any failure retains the evidence missing from the first
version25 test. Preserve the same SD data and transport timing throughout.

## Build and installed version26 results

The full `python3 scripts/checks.py` suite passed before deployment and again
after the hardware checks. The SDK reports1,412,672 program bytes against the
1,966,080-byte OTA partition, and122,864 bytes static RAM. The final image file is
1,412,816 bytes, SHA-256
`c4941a7408cf9503f6683c69ed3bb03926fe52a95a5053cf77a6ef2d3a7175f3`.
Its compiled history handler frame is544 bytes; the separate writer is1,504
bytes. The previous handler reserved3,232 bytes. Static RAM is unchanged.

Continuous OTA returned HTTP200, and the actual installed version was checked as
`1.10.26-history-stack-test`, initially paused/disconnected. All four matching SD
page/manifest assets passed SHA-256 readback. The private firmware image contains
network configuration and is not a public release artifact.

| Device check | Observed result |
| --- | --- |
| Paused, disconnected, one-hour rail |71 points,3.264 seconds |
| Paused, disconnected, one-hour rail/target pair |71/132 points,3.331 seconds |
| Manual engine, one-hour rail |72 points,4.952 seconds |
| Manual engine, one-hour rail/target pair, consecutive queries |72/129 points in5.290 seconds;73/130 in4.706 seconds |
| Manual engine, all-retained RPM/rail-target pair |102/196 points,6.661 seconds |
| Sensor JSON |49 entries parsed successfully |
| Automatic UART ownership |Invalid manual connection request rejected with HTTP409 `AUTO_MONITOR_OWNS_UART` |
| Automatic engine/EGS scheduling |Engine connected, EGS connected and persisted67 records, then engine reconnected |
| ABS qualification |Automatic qualification false; zero automatic connections |
| Storage at mode check |458 successful writes, zero read/write errors |
| Explicit pause |Automatic ownership released, session disconnected, polling stopped |

The first private reconnect assertion stopped after25 seconds, less than the
existing30-second delay after failed initialization. It was a failed test window,
not evidence of permanent logger failure. The device recorded one failed engine
initialization and subsequently recovered. The retained sticky monitor `error`
field still contained that older failure while current sessions succeeded; it
must not be interpreted as current connection state.

## Remaining response failure; candidate is not qualified

During a later paused, ordinary draining-client history request, the server
advertised19,138 body bytes but closed after4,308 bytes. Immediately afterward,
status remained available with continuous uptime, `httpTimeouts=1`,
`httpFailures=0`, and `httpMaxSendMs=250`. This proves the existing absolute HTTP
send deadline cut that response short. It does not establish why the socket
could not drain in time. Stack low-water remained3,484 bytes. No whole-response
allocation, malformed JSON length, SD error or reset was demonstrated in this
check. Automatic mode was resumed after this failed test; do not leave the
scanner paused because a verification script exits early.

The request harness's original `transferSeconds` includes waiting for headers;
it is not an isolated body-transfer measurement. Further checks now record
header latency, body-read latency and incomplete byte counts separately.
Accepted SDK WebServer sockets already enable TCP_NODELAY, so adding that option
again is not an evidence-based fix. Increasing the send deadline also increases
the maximum time this synchronous sender can hold the diagnostic loop; no such
change has been deployed. There is no retry or cached-history fallback hiding
the failed response.

The subsequent real dual-dashboard browser check timed out loading `/` before
either chart could be checked. At17:07 UTC, HTTP and ICMP to the scanner were
unreachable while the home router answered normally. A subnet check found no
new HTTP address for the scanner. Power/router-range conditions are awaiting
user clarification; this network loss is not attributed to firmware, brownout,
or the preceding send timeout without evidence. No further OTA or reset was
attempted while unreachable. Hardware stalled-reader and simultaneous-browser
qualification remain incomplete.

At17:09:47 UTC the scanner returned without intervention: uptime608,207 ms,
the same stack minimum and one recorded HTTP timeout. A subsequent check at
uptime648,131 ms showed engine connected, automatic ownership active,821
successful SD writes, zero read/write errors, two successful EGS connections
and95 persisted EGS records. Continuous uptime demonstrates no reboot during
that interruption, and the persisted counters demonstrate continued logging.
It does not distinguish radio/router reachability from every possible network
stack failure. Do not report the outage as a confirmed reset or ECU failure.

The read-only deliberately stalled-history probe subsequently obtained a200
header, but its health check timed out before a result could be recorded;
ordinary HTTP and ICMP/ARP checks also lost reachability. This hardware probe
has **not passed**. Physical power/range context remains unknown. The browser
check likewise has no passing live result in this session.

`tests/bounded_http.cpp` now reproduces temporary socket backpressure using the
actual paired JSON serializer: a pause after4,308 body bytes that clears at240 ms
finishes byte-exactly; an otherwise identical pause clearing at260 ms closes at
250 ms with an explicit timeout. It never resumes a failed response. The full
native/shared-client/dashboard checks pass with these regressions included.
These tests document the current deadline behavior; they do not repair its
tolerance for real network delays or qualify the device's close operation.

The retained version24 image remains the verified rollback; version26 is a
development test, not a production release. Its source and observed limitations
are retained on the development branch, separate from production/latest.
