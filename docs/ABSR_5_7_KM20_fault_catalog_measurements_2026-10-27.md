# ABSR_5_7_KM20 — verified ECU variant, CBF fault-code catalog and expanded measurements

Generated 2026-10-27 from the user's `ABSBR901(2).cbf` (SHA256 `3c9518b96ec61d6b43373147c7d13902b511214d6340cc94049ab4c303aa35fa`), Bosch variant patterns, the Autel trace, and the ESP32's read-only response.

**Variant identified:** `ABSR_5_7_KM20`. The captured `3C 00` response begins `7C 00 00 04 46 64 89`. The CBF variant-matching pattern contains ECU ID `00 04 46 64` and trailer `89`; this is the user's Bosch KM20 configuration. Do not apply the KM6 or ASG layouts without a fresh variant identification.

## Fault codes (definitions, NOT currently observed faults)

| Code | CBF meaning (English translation) |
| --- | --- |
| `C1000` | ABS/ASR controller internal fault |
| `C1010` | Battery supply undervoltage |
| `C1011` | Valve relay or valve power supply |
| `C1020` | CAN startup, transmission or reception |
| `C1022` | CAN IDs 210, 212, 218, 240, 308, 310, 312 or 608 |
| `C1024` | CAN ID 408 communication |
| `C1100` | Front-left wheel sensor circuit open |
| `C1101` | Front-right wheel sensor circuit open |
| `C1102` | Rear-left wheel sensor circuit open |
| `C1103` | Rear-right wheel sensor circuit open |
| `C1104` | Front-left wheel sensor implausible |
| `C1105` | Front-right wheel sensor implausible |
| `C1106` | Rear-left wheel sensor implausible |
| `C1107` | Rear-right wheel sensor implausible |
| `C1200` | Brake-light switch |
| `C1300` | Front-left inlet valve |
| `C1301` | Front-left outlet valve |
| `C1302` | Front-right inlet valve |
| `C1303` | Front-right outlet valve |
| `C1304` | Rear-left inlet valve |
| `C1305` | Rear-left outlet valve |
| `C1306` | Rear-right inlet valve |
| `C1307` | Rear-right outlet valve |
| `C1311` | Switchover valve 1 |
| `C1312` | Switchover valve 2 |
| `C1313` | Shutoff valve 1 |
| `C1314` | Shutoff valve 2 |
| `C1401` | Pump motor relay or motor supply |
| `C1407` | EEPROM shutoff byte checksum |
| `C1408` | Pressure sensor |
| `C1409` | Option coding |
| `C140A` | ASG present, no AAS |
| `C1410` | ASG present, no AAS |
| `C1411` | Retarder shutoff relay |
| `C1500` | Incorrect tone wheel or tire size |
| `P0000` | Retarder shutoff relay (CBF alias/placeholder; wire representation unverified) |
 
All 36 codes and the German original strings are in the locally extracted CSV catalog. These names are diagnostic-reference entries, **not** an indication the ECU actually has any of these faults. `C1401` specifically maps to *Motorrelais/Motorversorgung* (motor relay/motor power). The `P0000` CBF entry is unusual and must not be reported as a true engine DTC without confirmed wire encoding.

## Read-only ABS fault retrieval status

- Tried once with transmission-proven `18 02 FF 00` and once with standards-defined `18 00 FF 00`, after ensuring a fresh communicating session.
- The ABS responded `83 F3 34 7F 18 12 53`, i.e. `7F 18 12`, for both. This is **negative response, subfunction/parameter unsupported**. It is not equivalent to zero faults.
- No fault codes were cleared, no programming/coding written, no actuator routine transmitted.
- The exact ABS fault-memory request and response layout remain unverified by the CBF. The next external evidence needed is a passive Autel capture while displaying **ABS → Read Trouble Codes**; save the initial request and full multi-frame response.

## Read-only ABS live measurements — matched KM20 field map

| Diagnostic group | Fields |
| --- | --- |
| `2A 01 01` | Four wheel speeds; two-byte little-endian; raw 0–38 is stationary, 39–4800 times 0.0625 km/h |
| `2A 01 02` | Battery voltage (byte 3, times 0.071 V); wheel-sensor monitor (byte 4, times 0.0196 V); ABS-off analog (byte 5, times 0.02 V); UGD_I analog (byte 11, times 0.02 V) |
| `2A 01 03` | Brake-light switch, brake switch, pump motor feedback, all eight inlet/outlet valve feedbacks, two isolation/shutoff valves, two switchover valves, PAS input |
| `2A 01 04` | Engine RPM received over CAN (bytes 3–4, little-endian); accelerator pedal (byte 11, times 0.4%); four torque values in bytes 5–6, 7–8, 9–10, 12–13 |

### Captured group-4 read

```
TX 80 34 F3 03 2A 01 04 D9
RX 8F F3 34 6A 04 00 00 00 CC 08 C1 08 D2 06 00 D2 26 00 91
```

The first two data bytes represent **0 RPM**, and the pedal byte **0%**, with engine off. The torque raw values should not yet be interpreted as actual measured engine output: the CBF nominal scale gives implausibly high stationary values, so firmware displays raw until engine-running correlation.

The ECU's `3C 00` positive identification response was captured both with Autel and with the ESP32; it is safe/read-only. Existing ABS groups 1–3 have already been live-tested via ESP32.

### Safety

Normal scanner operation must not transmit `31` actuator routines, `3B` downloads, coding writes, `14` DTC erasure, or unverified ABS fault-read guesses. Other K-line modules retain their original framing.
