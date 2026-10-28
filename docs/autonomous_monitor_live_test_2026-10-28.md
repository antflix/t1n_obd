# First live autonomous test — 2026-10-28

**Outcome: PARTIAL PASS; auto-monitor disabled before continuing.** Test was executed remotely while user turned on the van, using firmware `1.10.20-autolog-test` on `192.168.8.168`. It began with monitor disabled, was explicitly enabled through `POST /api/monitor/enable`, then disabled again after repeated CR2 communication failures. Post-test status: `enabled=false`, `connected=0`, `polling=0`. No actuator commands, DTC clearing, factory resets, card formatting, or firmware flash occurred during this live test.

## Confirmed real data

- CR2 engine **5 successful sessions**, **593 logged recent samples**, 6 baseline DTCs. Actual SD history data and timestamps were readable for `rpm`, `coolant`, `rail`, `railTarget`, `battery`, `boost`, and `speed`; sample values included 643–1,487 RPM during test and speed up to 28 km/h. Automatic reconnect succeeded multiple times, but stability was inadequate.
- EGS transmission **1 successful session**, **64 logged recent samples**, 3 baseline DTCs. Readbacks confirmed `temp` 48°C, `battery` 14.075 V, `speed` 0 km/h, and `outputRpm` 0 during its sampled window. The scheduler switched back to engine successfully.
- ABS: **1 attempted session, 0 successful**, no samples. K-line receive pin **GPIO33** remained HIGH while transmit GPIO32 was commanded LOW. No StartCommunication response bytes. This is evidence to investigate the pin 9 transceiver's enable, supply, ground, driver, and TX/RX path before altering K-line protocol requests. It does not by itself isolate which hardware component failed.
- HVAC/HZR: **not visited** because the test was stopped due to repeated CR2 communication failures.
- End-of-test microSD: `recent=657` records, `15m=188`, `daily=113`; **1,068 successful journal writes** including summaries and earlier synthetic self-tests; **0 SD write errors**, **0 sensor registry overflows**, `clock.valid=true`, 0 detected jumps. Module IDs / recent logging counters: engine 0 / 593, EGS 2 / 64, ABS 1 / 0, HZR 3 / 0.
- DTC-change event counter 9 = engine initial 6 + EGS initial 3; no additional transitions confirmed in this test. Baselines are not new fault alerts. ABS/HVAC DTC reads still unsupported. DTC memory was not cleared.

## Instability observed

- CR2 **6 failures** total, including three lost sessions after three consecutive read-only diagnostic requests failed; several quick reinitializations saw only echoed TX bytes without an ECU reply. Automatic recovery occasionally succeeded after additional backoff. Repeated retries are not acceptable as a permanent autonomous solution.
- ABS drive-low observation: `RXD levels: idle=HIGH, driven low=HIGH, released=HIGH` (RX GPIO33/TX GPIO32). Contrast CR2 `idle=HIGH, driven low=LOW, released=HIGH`, showing correct low-drive behavior there.
- Until channel hardware and real bus timing are diagnosed, do NOT blindly increase retry frequency, claim all four modules are logging, or release the scheduler as unattended/production stable. Keep default disabled. The main assumption that all four channels are ready for unattended use is contradicted by this vehicle test.
- Vehicle history recorded movement up to 28 km/h; ABS scheduler correctly attempted its visit only when its fresh stationary check allowed it. Future diagnostic mode tests should be performed stationary.

## Captured private evidence on Home Assistant computer

- `/home/ant/t1n-stage2-live-ecu-test-20261028.json` — final status, counters, 46 timed observations, selected activity failure lines.
- `/home/ant/t1n-live-monitor-20261028.jsonl` — 46 periodic telemetry snapshots.
- `/home/ant/t1n-live-activity-20261028.txt` — vehicle session trace.
- `/home/ant/t1n-live-raw-log-20261028.txt` — K-line raw request/response tail; do not publish without privacy review.
- Previous full 4 MiB backup and 1,904 legacy historical records are preserved separately; no SD contents were erased.

## Next gated work

1. Inspect/test ABS TLIN channel with scanner disconnected from vehicle: verify 12V input/supply, enable pin, shared ground, TX GPIO32-to-transceiver input and RX GPIO33, then verify the transceiver can pull its own K-line output LOW without the van attached. Compare to working engine channel.
2. Analyze raw CR2 command/response timestamps and polling configuration to distinguish ECU timeouts, electrical interruptions, and SD-writing-induced scheduling delays. Instrument per-request failure reason and maximum SD-write latency before changing protocol timings; add a failure-budget circuit breaker so repeated errors disable automatic scanning.
3. After those fixes and stationary confirmation, run a bounded engine/EGS reliability test, then ABS and HVAC one at a time, confirming real records and reconnect behavior.
4. Confirm offline time persistence with a battery-backed RTC before claiming reliable timestamps across power-off; currently network-synced time was verified only.
5. After stable readings, build context-aware warning/critical alert rules and the separate dash display.

See [draft PR #3](https://github.com/antflix/t1n_obd/pull/3). Do not merge until the above real ECU reliability gates pass.
