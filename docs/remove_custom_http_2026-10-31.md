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
dispatch pass. Version 29 has not been built or installed by this change.
