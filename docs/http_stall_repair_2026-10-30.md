# HTTP stalled-reader repair — 2026-10-30

The user reported that opening rail history failed and the scanner stopped responding, requiring two manual reboots on `1.10.20-chart-ui-test`. Read-only inspection after the latest reboot found HTTP responding, uptime approximately 131 seconds, engine disconnected, autonomous monitoring disabled, and zero new history writes since boot. No reset-cause telemetry existed in that image; a spontaneous firmware reset has not been established.

## Reproduced failure

With diagnostics already disconnected, opened one `/api/sensors` TCP request with a 1,024-byte receive buffer and clamped receive window, deliberately leaving the response unread. A second `/api/status` request timed out after three seconds. Closing the stalled reader after four seconds restored HTTP in approximately 40 ms, with uptime continuing. This demonstrates HTTP-loop starvation without requiring a reboot or ECU fault.

Arduino ESP32 core 3.3.12 `NetworkClient::write()` uses up to ten one-second `select()` waits. Partial progress resets its retry counter. `WebServer::sendContent()` ignores short writes, and each 256-byte JSON chunk incurs multiple core writes. The sensor and completed-history responses therefore had no total write deadline. The former bounded JSON encoder bounded memory, not socket blocking time.

## Repair in 1.10.21-bounded-http-test

- One fixed-length HTTP response with raw `MSG_DONTWAIT` sends and a 250 ms absolute write budget, including all chunks. Partial writes retry within that same budget. Failure closes the socket and suppresses all remaining output. No response-sized heap buffer and no repeated chunk-header allocations.
- One serializer counts JSON bytes and emits them. Sensor timestamps and stale classification use a captured snapshot time, preserving identical output length across both passes.
- The same bounded adapter serves sensor/history JSON, read-only text/JSON responses, SD pages/downloads, and embedded HTML/JavaScript. Download disposition and compressed-asset encoding remain explicit. Firmware upload behavior and diagnostic transport are unchanged.
- `/api/status` now exposes uptime, reset reason, free/minimum/largest heap, HTTP timeout/failure counters, and measured maximum response duration. Duration includes socket cleanup; OS/SD operations are not claimed to have a hard 250 ms bound.
- One-hour history reads skip quarter-hour/daily journals; 24-hour reads skip daily. The existing range/tier semantics and retained records are unchanged.

Native regressions cover partial writes, permanently blocked peers, mid-body stalls, slow progress across chunks, disconnects, exact Content-Length including >8 KB JSON, empty bodies, clock rollover, content headers, and close duration. Production history queries run through the same JSON wrapper in native tests, including tier boundary cases. `scripts/check_http_responsiveness.py` repeats the GET-only stalled-reader experiment against installed firmware and checks health latency, uptime continuity, timeout counters, and sensor recovery.

Separate known risk: the existing logger can synchronously flush many sensors at quarter-hour/daily boundaries. That SD latency is not repaired or hidden by this networking change. Extended hardware reliability still requires measurement across such boundaries.

Installation evidence will be appended after the real stalled-reader and browser checks.

## Hardware iteration

Installed `1.10.21-bounded-http-test` through the existing OTA route; matched SD pages passed SHA-256 readback. Three non-draining clients while diagnostics were disconnected produced HTTP status latencies of 25, 28, and 22 ms, observed maximum send duration 251 ms, and continued uptime. The prior firmware timed out after three seconds under the same condition. Engine quick initialization then succeeded. A real mobile browser loaded both rail series and the selector controls with zero POSTs or JavaScript errors.

Active polling exposed additional response deadlines (30 total after only four deliberate stalls). A repeated active-session test saw a status request take 1.021 seconds, followed by a transient connection timeout; the scanner subsequently responded without reset, engine still connected. This is not recorded as a fully successful loaded-system test. Native socket tests alone did not cover real packet queue behavior.

The accepted sockets already have TCP_NODELAY enabled. Sending each 256-byte encoder flush separately wastes the limited lwIP packet queue (configured send buffer 5,744 bytes, MSS 1,436 bytes). `1.10.22-http-packets-test` therefore uses a fixed 1,436-byte JSON buffer and file chunks, keeping the same absolute deadline. Counter/output serializer lifetimes are separated to limit stack use; stack minimum telemetry is added. The response format and logging policy do not change. Regression tests require MSS-sized output call counts for a 12 KB JSON response.

Before installation of this follow-up, the ESP32 stopped responding even to ICMP, while the router remained reachable. Power/network state confirmation was requested; this is separate from declaring the HTTP repair verified under active load.

## Scope narrowed at the user's request

A comparison confirmed that 1.10.19 to 1.10.20 changed only mobile UI, version and tests/notes; their sensor/history streaming backends were identical. The new loading message was not the failure; chart retrieval blocked, and the sender could stall the shared loop.

Removed the broader response conversions from this repair: SD storage/page/download code and setup routes are restored byte-for-byte to 1.10.20. Removed generic HTML/text/file/gzip/header output helpers and the tier-skipping optimization. The candidate `1.10.23-narrow-http-test` replaces only the sensor and completed-history output paths, using the same JSON schema, captured sample time, fixed MSS buffer and bounded socket sender. It retains minimal reset/heap/stack/HTTP telemetry for verification. There is one sensor/history sender, with the old chunked path deleted; no fallback or competing implementation remains. Existing asset loaders and logger/scheduler/ECU protocols are unchanged from the working baseline.

Version22 hardware checks passed stalled sensor readers both idle and active (active status replies14ms), and a stalled completed24h rail-history response (14ms), with continuous uptime/engine polling and persisted SD writes. This short interval is not treated as equivalent to the user's previous hour-long run. Version23 will repeat these targeted checks after installation.
