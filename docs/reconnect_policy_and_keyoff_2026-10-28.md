# Field verification: consecutive engine retry and key-off, 2026-10-28

Current installed firmware 1.10.21 had disabled automatic monitoring after 3 engine failures within 20 minutes. It had successfully recorded 151 recent engine samples before a session was lost and two later reconnect attempts failed. This automatic permanent disable was too aggressive. User requested three consecutive failures before cooldown instead of cumulative failures.

Replacement 1.10.22 test code: 15s delay after one consecutive engine failure, 45s after two, 120s after three, 300s after four or more. The retry policy does not disable logging; it continues later retry attempts. A successful real recorded engine sample clears the streak. API exposes streak, next retry, last successful sample, and recoveries. Local native tests passed; CI and device validation required.

Ignition observation: a 97-second polling window after user announced key off showed both engine GPIO13 RX and EGS GPIO26 RX remained HIGH, and ignitionSignal remained HIGH. The actual ignition position was not independently visible remotely. A transceiver being able to pull the bus low during ECU communication does not demonstrate its RX output stays low when the key is off; receiver pullups are enabled. Accordingly the current two-pin software ignition gate is NOT verified as a key-off sensor. Do not claim its key-off shutdown works; consider a protected switched ignition sense input.

Evidence on Home Assistant: /home/ant/t1n-ignition-1.10.21-verified/key-off-observation-20261028.jsonl. Current device monitoring disabled automatically via legacy circuit breaker, engine disconnected. Retained SD logs unaffected.
