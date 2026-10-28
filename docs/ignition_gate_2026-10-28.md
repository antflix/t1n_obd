# Ignition-gated logging: stage-3 test build and verification

**Source:** `feature/autonomous-sd-monitor-20261028` version `1.10.21-ignition-gate-test` — staged but **NOT installed on the van** during the 2026-10-28 running-engine test. Installed firmware remains `1.10.20-autolog-test`, with auto monitor user-enabled and no ignition-off gate.

## Verified running-van experiment

After changing the scanner's NVS `pollIntervalMs` from 300ms to **1000ms after each completed diagnostic request**, enabled autonomous monitor with the van parked and running. Remote 10-second supervisor held a cutoff at two new engine failures or two SD write errors.

**Result: 226.6 seconds, no new engine failures and zero SD write errors.** Later continued monitoring reached engine 9 connections (6 historical failures, **no new engine failures** in the slower-poll test), EGS 3 connections, and ABS **1 successful connection after 1 prior failure**. ABS stored 14 real samples, including 0 km/h wheel speeds and ~14.058 V module voltage. Because its first attempt had failed electrically, its automatic qualification remains pending further stable repetitions. Live SD writes exceeded 2,100 with zero write errors. Engine connected successfully, EGS was visited and logged new samples, then engine reconnected; baseline counters changed from engine connects=5/failures=6/historyWrites=593 and EGS connects=1/historyWrites=64 to engine connects=7/failures=6/historyWrites=761, EGS connects=2/historyWrites=118. All four passive RXD inputs read HIGH during ignition ON. The scanner remained enabled and connected after the supervised window; this is a bounded observation, not proof of indefinite stability.

Private proof: `/home/ant/t1n-live-1000ms-supervised-20261028.jsonl`. No DTC clearing, actuator requests or new firmware flashing during this live test.

## Prepared ignition logic, NOT hardware-qualified

- Passive read of CR2 RX GPIO13 and EGS RX GPIO26 (no extra GPIOs or power wiring).
- A candidate IGNITION ON requires **both inputs HIGH continuously for at least 3 seconds**.
- A candidate IGNITION OFF requires **both inputs LOW continuously for at least 8 seconds**.
- Mixed signals do not change the last valid state.
- New ECU initialization is blocked while state is LOW or UNKNOWN. On sustained LOW, background diagnostic polling/session is released without sending a disconnect frame to potentially unpowered modules. On HIGH, eligible automatic monitoring can resume.
- Safety circuit breaker disables automatic monitoring after **three CR2 session/connection failures within 20 minutes**, leaving an error indication. ABS automatic visits are disabled until its previously observed failure to pull GPIO33 LOW is repaired. Manual ABS diagnostic controls remain separate.
- Native tests of LOW/HIGH/mixed ignition gate and DTC comparator: PASS on Home Assistant PC.
- **Important:** RX inputs are configured `INPUT_PULLUP` for open-drain TLIN1027 transceivers; HIGH alone does not electrically prove ignition ON. The user's claimed LOW when ignition OFF still requires real vehicle validation, including timing after key-off and comparison of both actual RX GPIO values. Loss of Internet/RTC and ECUs staying awake can affect reliability. Do not call this an ignition-qualified production system yet.
- Do not OTA-flash the new gate while the van is running; run off-level capture first, then install and validate on/off transitions and stored NVS enable state.

## Remaining acceptance tests

1. Coordinate with user to disable the **currently installed** automatic monitor via `POST /api/monitor/enable enabled=0`, then turn key OFF and query `/api/hardware/lines` repeatedly for 15–30 seconds. Verify both engine and transmission RXD signals actually fall LOW for >=8 seconds, while scanner remains powered.
2. If both signals do not fall LOW, design a dedicated fused IGN/ACC sense input instead of trusting RXD pull-ups. Do not force an unverified rule.
3. With key off, checksum-verify isolated 1.10.21 OTA artifact and install. Reboot, confirm SD history and clock status, set auto enable once safe and inspect `/api/monitor/status` ignitionSignal and ignitionLowObserved.
4. Test on/off/on across at least two cycles with no ECU requests while OFF, predictable resume after ON, recovered engine/EGS sessions and SD samples. Observe 20+ minutes for failures, review error budget.
5. Requalify ABS transceiver electronics before scheduling ABS visits. HVAC remains pending; do not imply all four ECU metrics are proven.

All existing history (including 1904 recovered earlier journal records) and full 4 MiB flash backup remain preserved off device.
