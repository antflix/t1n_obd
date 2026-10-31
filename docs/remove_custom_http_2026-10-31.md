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
