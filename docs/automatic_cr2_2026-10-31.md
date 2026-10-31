# Automatic CR2 operation — 2026-10-31

`1.10.30-automatic-cr2-test` removes browser/manual ownership of diagnostic
communication. Automatic monitoring starts one second after boot, independent
of any previously saved enabled=false setting and without a browser request.

The existing scheduler is now the only connection/retry owner. A failed engine
connection waits 30 seconds and retries indefinitely; a lost established
session waits 10 seconds before reconnecting. No failure-count lockout is added.
The original verified quick initialization, optional post-C1 handoff and 21 12
live-response verification remain. The disabled full replay fallback and its
unused receive-capture helpers are removed, along with the old failure path
that set automatic reconnect to false. ECU response deadlines, waveform,
GPIO routing and polling pause are unchanged.

Deleted the five-minute manual override, persisted monitoring enable/disable
setting, manual connection queue and parallel reconnect mechanism. The mobile
interface and embedded recovery page no longer offer connection, disconnection,
start/stop polling, engine active tests, manual fault scans/clearing, raw payloads
or transport-setting controls. Advanced is a read-only system/log page with
Wi-Fi and firmware-update links. Chart selection, clock sync, exports and log
controls remain available. Old/manual UART request paths uniformly return HTTP
409 AUTOMATIC_MONITORING, including between connection attempts; they cannot
stop the logger or seize the UART. Their old action implementations are deleted.
The CR2 actuator-test file, manual clear-DTC and raw-payload implementations
are also removed.

Existing scheduled EGS/HVAC visits and engine priority remain. All physical
interfaces still share one UART, so these short visits temporarily leave the
engine session. The old automatic ABS path is removed because that interface
has not been qualified for unattended operation. This change does not promise
simultaneous module sessions or successful ECU communication with ignition off.

History continues to use the existing SD journals exclusively. No new HTTP
transport, response cutoff, task, queue, persistence format or Pi database is
introduced. SDK HTTP handling remains in the main loop outside ECU timing waits.

Native tests compile production monitor functions and API rejection registration.
They cover browser-free startup, 200 failed attempts then recovery, lost sessions,
retry boundaries, busy state, uptime wrap, secondary visits, no automatic ABS,
and HTTP409 with no diagnostic state changes. Existing JSON, SD failure/checksum
integrity recovery, rollover, clock/range, chart and browser tests remain.

The development branch is included in the existing GitHub Actions build trigger.
It uses the pinned ESP32 3.3.12 SDK and keeps test artifacts separate from the
production latest release. Build, image checksums, installation and hardware
results must be recorded after actual completion; source tests are not proof
of live ECU continuity.
