# Autonomous ownership audit and repair — 2026-10-31

Audited baseline: `5dc0ad1`, branch `feature/shared-sd-data-20261030`, firmware `1.10.24-history-read-test`. This audit made no device requests, changed no saved configuration and started no ECU sessions. At handoff, the parent agent reported the engine manually connected/polling and automatic monitoring disabled. That live state must not be confused with a qualified autonomous scheduler.

## Why the intended ownership architecture was missing

The initial-SD rollback deliberately restored the older scheduler. Subsequent shared-history/HTTP repairs preserved it. Baseline `38_autonomous_monitor.ino` therefore still contained `monitorManualOverride`, `monitorManualHold`, a five-minute hold timer, and an indefinite yield to an existing manual session. `90_setup.ino` allowed manual Connect, Disconnect, poll controls, raw diagnostic requests, configuration changes and active tests while automatic mode was enabled.

Historical commit `41b7899` introduced explicit automatic ownership and removed the hold. Staged `16c1117` added HTTP 409 checks and automatic-mode UI controls; see `docs/autonomous_ui_single_owner_2026-10-29.md` on `origin/feature/autonomous-sd-monitor-20261028`. Those commits are historical evidence, not safe wholesale replacements: they also contain the unqualified K-line ignition gate, and their route checks miss several legacy aliases such as `/payload`, `/dtc/clear` and `/dtc/read`.

The reverted scheduler also scheduled ABS automatically after a fresh stationary-speed/RPM check. Being stationary does not qualify its intermittent physical channel. This conflicted with the later requirement to keep unattended ABS disabled.

## Targeted source repairs

- Replaced the hold timer and separate ownership booleans with the small portable `MonitorOwnership` policy used directly by production code. Automatic mode owns the UART between sessions and during backoff; session ownership separately determines which session an explicit pause releases.
- An explicit enable releases a preexisting manual session and schedules engine acquisition. Pause releases a logger-owned session and clears manual reconnect state. Repeating the same mode request leaves an already-running session unchanged. No timeout silently returns manual mode to automatic.
- An unexpected legacy session cannot cause enabled monitoring to yield indefinitely.
- Removed the unattended ABS visit entirely. `monitorStart` also enforces the shared qualification policy: engine/EGS/HZR only. Manual ABS support is unchanged. KI/EWM are not added to unattended visits.
- Held the existing `busy` flag across session release, and `jobConnect` across the entire automatic connection handoff. This matters because transport waits service HTTP, and `connectFastModule` clears `busy` before engine post-initialization verification finishes. A nested HTTP mode change must not replace the transport mid-operation.
- Monitor status now names `uartOwner` and reports per-module `automaticQualified`; removed the obsolete `manualHold` field.

The parent integration adds a single manual-route guard (HTTP 409 in automatic mode, or while busy/connecting), covers legacy aliases, updates the forward declarations, and keeps the main-loop manual connect-job flag held until `connectModule` returns. Read-only page, sensor, history, module catalogue and fault-display GETs remain available. No K-line transport waveform, ECU initialization payload or diagnostic timing changes are part of this repair.

## Existing behavior retained and remaining gaps

| Area | Verified source behavior | Remaining work |
| --- | --- | --- |
| Startup | `monitorBegin` loads `t1nmonitor/enabled`, default false, then waits 12 seconds after monitor initialization before acquisition | Test enabled boot and pause persistence on hardware; documentation saying enabled-by-default is obsolete |
| Engine reconnect | Failed engine initialization waits 30 seconds; owned session loss waits 10 seconds; failed secondary visit returns to engine after 1 second | Benchmark failure/recovery durations; restore proven consecutive-failure policy separately, without the ignition gate |
| Persisted recovery | `monitorPersistedSample` only increments module write counters | No last-successful-sample age or streak recovery yet; clear a future retry streak only after a real persisted sample |
| Poll rate | Compiled default remains 300 ms; `cr2scan/pollint` NVS overrides it; next poll is scheduled after the completed request | Confirm actual saved value before changing it. The documented 1000 ms experiment is an inter-request pause, not a fixed sample rate |
| Secondary scheduling | Engine minimum 120 seconds; EGS 12-second visit roughly every 4 minutes; HZR 8-second visit roughly every 15 minutes | Measure actual switching/request overhead and each module's fresh saved samples before unattended qualification |
| Status | Connection/failure/write counts and last-connection uptime exist | Add last real contact/sample, next visit and retry reason when implementing the measured retry policy |
| Ignition | Current baseline has no RX-level ignition gate; all four RX inputs use pull-ups | Keep ignition unqualified; dedicated protected ACC input needed. HIGH is not proof of key-on |
| Clock | History requires valid UTC and preserves sample receive time; uptime is separate | RTC required across complete power loss without synchronization |
| Event writes | Monitor/DTC CSV writes are synchronous; monitor clock heartbeat still runs every 5 minutes even while automatic mode is disabled | Measure maximum write latency; avoid claims that every SD operation is bounded or that diagnostic and HTTP execution are independent tasks |
| Storage | Recent sample persistence calls `monitorPersistedSample` only after successful SD append | Unknown clock/storage failure must not masquerade as persisted recovery |

The historical `monitor_retry_policy.h` at `16c1117` uses 15/45/120/300-second consecutive-engine backoff and clears its streak only on a persisted real engine sample. That is a useful separately testable follow-up. It is deliberately not mixed into this ownership repair before timing measurement. The current scheduler does not permanently disable monitoring on intermittent failures.

## Wiring/configuration audit

`HardwareSerial KL(2)` is the sole diagnostic UART. `15_relay_router.ino` fixes physical channels to RX/TX `13/23`, `14/27`, `26/21`, `33/32`, mapped to OBD `7`, `15`, `11`, `9`. Channel selection ends UART2, drives TX pins recessive, and attaches UART2 to the selected pair. No simultaneous sessions are implemented.

The initial `TX_PIN=27` / `RX_PIN=14` declaration and old `ScannerConfig.relay*` fields are legacy/misleading: `setup` calls `initRelayRouting`, which selects engine `13/23`; actual routing uses the fixed channel arrays, not saved relay GPIO settings. Do not change wiring or GPIO defaults as part of ownership work. Removing the obsolete relay configuration fields is separate UI/configuration cleanup.

## Tests and qualification status

`tests/autonomous_state.cpp` compiles the actual portable production policy and verifies default/manual mode, resume with a manual session, enabled ownership without a session, idempotent enable, pause/release, idempotent manual mode, persisted-enabled startup semantics, and the unattended module allowlist including explicit ABS exclusion. Native GCC warnings-as-errors plus AddressSanitizer/UndefinedBehaviorSanitizer passed.

`tests/autonomous_runtime.cpp` includes the actual production `38_autonomous_monitor.ino` with a process-local NVS fake, in-memory SD fake, simulated monotonic time and lower-level transport callbacks. It verifies enabled/default-paused boot, the exact 12-second boot delay, no fabricated session at boot, mode persistence, resume/release of a manual session, idempotent enable, no timed return from manual mode, the actual ABS/KI/EWM connection-entry gate, engine/EGS visit/return, unchanged 10/30-second retry paths, recovery, and reclaiming an unexpected legacy manual session. The callbacks simulate nested HTTP attempts during disconnect and the fast-init interval after `busy` clears: pause holds `busy` even after mode becomes manual, and automatic connect holds `jobConnect` until live-reply verification returns. These tests passed with GCC warnings-as-errors and AddressSanitizer/UndefinedBehaviorSanitizer. Storage-selftest-only dependencies are link stubs; this harness makes no claim about their correctness or real K-line electrical behavior.

This is source and native-test verification. Full integration checks, ESP32 compilation, size verification, deployment, HTTP 409/device mode transitions, reboot persistence and real engine/EGS scheduling remain the parent agent's qualification steps. No unattended monitoring, ABS qualification, ignition qualification or power-loss clock continuity is claimed here.
