# Remove the custom HTTP sender

`1.10.28-sdk-http-test` starts from rollback commit `d4f7791` and restores
the version 19 SDK chunked, single-pass sensor/history sender (`6069481`).
The custom raw-socket sender, two-pass length calculation, 250 ms whole-response
deadline, transport metrics, and their obsolete tests/probe are removed.
The original 256-byte JSON encoder remains allocation-free in application
code and still escapes strings and streams responses larger than 8 KB.
Sensor age fields retain the version 24 single monotonic snapshot.

Version 24 sector-based cooperative history reads and UI fixes are retained.
No Wi-Fi behavior, retry timing, ECU commands, UART ownership, SD format,
or dashboard data flow changes are included. Historical charts remain SD-only.
Existing uptime, reset-reason, heap, and stack status fields are retained.
The separate temporary network trace is not included.

`python3 scripts/checks.py` passes, including production history code sending
a complete 200-record response through a simulated draining SDK client taking
more than 250 ms. The fixture checks the final empty chunk, exact body,
record count, and maximum 256-byte chunks. Existing storage fault/recovery,
rollover, sector-read, range/clock, large JSON, shared-client, mobile and Pi
tests also pass. `git diff --check` passes.

This removes measured JSON stack pressure and the artificial response cutoff;
it does not establish the cause of network loss. The SDK sender is synchronous:
a genuinely stalled peer can delay the main loop under SDK socket timeouts.
The SDK also uses small internal chunk-header allocations. There is no claim
of subsecond service while a peer is stalled. Build and hardware qualification
remain outstanding for this isolated source commit.

## Version 28 hardware result and version 29 isolation

Version 28 was installed and its sensor JSON parsed correctly. During the
two-browser chart check, the ECU session was lost around uptime 86 seconds
and recovered around 90 seconds. Further losses were observed around 101,
126, 151 and 183 seconds, without another diagnostic POST after the initial
connection. Later HTTP requests timed out at five seconds while ARP remained
reachable; an activity-log GET also succeeded. Version 28 is not qualified
as a stable repair. This evidence does not establish one cause for every
network interruption.

Version `1.10.29-uart-http-isolation-test` removes the six nested
`web.handleClient()` calls from transport receive, P3, replay and post-C1
waits. The post-C1 helper is renamed from `waitMsWeb` to `waitMs`; all
durations, delays, byte sequences and ECU commands remain unchanged.
Only the main loop dispatches HTTP work. No new task, queue, timing policy,
reconnect logic or observer is added.

The native regression compiles the current production receive functions
and frozen verbatim version 28 functions. A simulated 250 ms HTTP callback
causes version 28 to miss a complete reply arriving at 90 ms within a 100 ms
receive deadline. With HTTP removed, production receive captures the exact
frame at 90 ms, and raw receive completes its five-ms quiet period at 95 ms.
It also checks no-data/late/corrupt replies, buffer bounds and clock rollover.
This demonstrates a real timing interference path; hardware qualification
must establish its contribution to the observed failures.

HTTP requests now wait for diagnostic transactions and initialization to
finish. Long connection attempts can therefore delay the dashboard. This
is the explicit limitation of retaining the existing single-loop design.
Full offline checks and the source contract requiring one main-loop HTTP
dispatch pass. The results below were collected after this source change.


## Version 29 build and actual installation

Built source commit `439be0cd403c70fc5a60047642519b68624a98a6`
(public equivalent `a5ba626610c27d88232e52f0c7e76fbea8ca3546`, tree
`65d4622630a761e8a3aeec118a5b672be0dc88a8`) with Arduino ESP32 3.3.12,
`esp32:esp32:esp32:PartitionScheme=min_spiffs`.

- Image: 1,403,824 bytes; OTA capacity: 1,966,080 bytes.
- Image SHA-256:
  `cba067aa8a9d6ac6fbc30dafedd8ab7c0a2cf687bb5f344d3814feae13c98fb9`.
- Matching SD bundle SHA-256:
  `7d02964265689c3546e8262804195f937c63bdc997384ff4fc643bd3838720d8`.

USB installation used the Pi's CP2102 connection. Before writing, the target
MAC and existing 4 KiB partition table were checked against the build.
Only the app0 application and 8 KiB OTA boot-selection region were written;
esptool verified the flash hashes. NVS, bootloader, partition map, microSD
journals and unrelated card files were preserved. No full-flash backup or
repartitioning was performed. Existing HTML pages already matched their
SHA-256 values; only the 441-byte version manifest needed updating on SD.
Installed firmware version and page hashes were checked afterward.

The initial engine connection received only local transmitted echo
`81 12 F3 81 07`, without a StartCommunication reply. The UI reported the
failure and remained available. Physical ignition/OBD conditions were requested
from the user and are pending. Do not infer a firmware regression or a hardware
fault from that result without the physical conditions. No additional diagnostic
commands were sent after the single initial connection request.

## Verified scope and outstanding qualification

- Full native/shared-client/mobile/Pi fixture suite passes, including the old
  missed-reply regression and complete responses over 8 KiB.
- Local browser fixtures pass in Chromium and WebKit for history reload,
  pending/error/empty/limited responses, scanner/ECU independence, stale data,
  offline/recovery and read-only navigation. These are fixtures, not physical
  iPhone Safari verification.
- A real 375-pixel Chromium browser loaded both 200-record rail series from
  microSD while the ECU was disconnected in 8.239 seconds. The metrics picker
  opened and closed with Done. No failed requests, JavaScript errors or POSTs.
- A separate real browser reload check loaded the paired chart twice in
  9.440 and 8.933 seconds and received exactly identical persisted points.
  No browser-generated samples or diagnostic POSTs were involved.
- The actual Pi tab loaded in 285 ms. Over two minutes, 93 completed API
  responses were HTTP 200 and valid JSON, with no network failures, exceptions
  or POSTs. Scanner online, ECU disconnected, gauges stale/unavailable, SD
  history available, storage healthy and clock valid stayed distinct.

Engine continuity, continuous SD writes under two-browser load, and long-run
network reliability remain unqualified until the powered ECU can be reached.
Automatic module monitoring remains disabled for this baseline isolation;
no unattended ABS or secondary module visits have been enabled. Chart loading
still needs optimization after stable logging is established. Production
release/latest remains untouched; this is a development branch/build.

After the build, an obsolete history-source comment claiming that readRaw still
dispatches HTTP was removed. That is a comment-only correction and does not
require another flash. Private images, network configuration and raw logs remain
outside the repository.
