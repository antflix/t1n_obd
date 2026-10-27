# ABSBR901 — captured Autel read-only session, 2026-10-27

## Provenance
- Original logic analyzer: Saleae Logic via `sigrok-cli -d fx2lafw` on the user's Home Assistant machine.
- Raw recording: `/home/ant/logic-captures/abs_live.bin`; sigrok session: `/home/ant/logic-captures/abs_live.sr`; text UART transcript: `/home/ant/logic-captures/abs_live_uart.txt`.
- 60,000,000 samples at 1 MHz. Only channel D0 toggled (6,114 edges); all D1–D7 remained high.
- Decoded UART: 10,400 baud, 8N1, non-inverted. 1,423 bytes decoded by sigrok; 130 complete valid-checksum KWP frames identified.
- Capture made with the Autel scanner on the vehicle OBD port and the custom board passively monitoring OBD pin 9 through TLIN1027 RXD.

## Connection
Autel sends compact-frame StartCommunication:
```
TX 81 34 F3 81 29
RX 83 F3 34 C1 43 46 F4
```
**Critical correction:** The exact `43 46` reply previously received by the ESP32 is **also** accepted by the Autel in this successful live-data session. It was incorrect to infer the earlier `43` vs CBF-configured `C2` discrepancy itself prevented ABS polling.

Subsequent Autel requests use KWP's **explicit-length frame form**, i.e. format byte `80`, address pair `34 F3`, one separate length byte, data bytes, additive checksum. The ESP32's existing `buildKwpFrame()` uses the compact `80|payloadLength` format. Autel TX bytes are separated by approximately **5.95 ms start-to-start** (matches ESP32 `cfg.byteSpacingUs=5952`). The ECU response bytes are closer to ~1.4 ms start-to-start.

Additional preliminary Autel exchange:
```
TX 80 34 F3 02 83 00 2C
RX 87 F3 34 C3 00 0A 02 6E 14 0A 09
TX 80 34 F3 02 3C 00 E5
RX 8F F3 34 7C 00 00 04 46 64 89 00 00 03 42 01 40 01 08 F8
```
Autel then sent an extensive `3B 00 ...` request (107-byte payload), and several `31 23 ...` requests, with `7B 00`, `73 23 00` and a negative response seen. **Do NOT blindly replay these:** `3B` and `31` can be state-changing services. Whether they are required for read-only `2A` access is not established.

## Working live-data traffic

### Group 1 — wheel speeds
4 successful read transactions:
```
TX 80 34 F3 03 2A 01 01 D6
RX 8B F3 34 6A 01 00 17 00 17 00 17 00 17 00 79
```
The ECU's diagnostic payload is `6A 01 00 17 00 17 00 17 00 17 00`. The CBF defines wheel speed fields at payload byte offsets 3,5,7,9, width 2. The recorded data bytes in these four fields are `17 00`. **Little-endian** gives 23 counts, inside the 0..38 deadband (0 km/h); big-endian would give 5888 counts, beyond the CBF's maximum of 4800 and thus nonsensical. Wheel speeds all decode as 0 km/h.

### Group 2 — analog / voltage
10 successful transactions:
```
TX 80 34 F3 03 2A 01 02 D7
RX 8C F3 34 6A 02 00 B9 AF 01 00 00 FF 84 03 00 0E
```
The CBF says payload byte 3 = `B9` = 185 * 0.071 = **13.135 V** terminal supply; byte 4 = `AF` = 175 * 0.0196 = **3.43 V** wheel-speed sensor analog input. Further analog values are variant-specific; do not assert their meaning without identifying the ECU variant.

### Group 3 — digital switch and feedback
9 successful transactions:
```
TX 80 34 F3 03 2A 01 03 D8
RX 96 F3 34 6A 03 00 00 FF 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 29
```
The CBF defines byte 3 = brake-light switch, `00` = not pressed; byte 4 = separate brake switch, `FF` = not pressed. The pump motor feedback at byte 6 = `00` (stopped). These appear consistent with a stationary vehicle. No brake actuator commands were sent by the custom reader.

### Additional ECU frames
The ECU also emitted `83 F3 34 6A 0D 00 21`, and during repeating group-2 and group-3 reads, `83 F3 34 6A 83 00 97` and `83 F3 34 6A F3 00 07`. Their diagnostic semantics require further analysis and should not be represented as measurement responses for groups 1–3.

## Firmware implications

Prior code had three demonstrated problems:
1. `sendKwpPayload` for ABS sent compact frames (e.g. `83 34 F3 2A 01 01 D6`) while the accepted Autel request includes an explicit length byte `80 34 F3 03 ...`.
2. `absPayload` rejected valid replies by expecting `6A 01 <group>`; actual header is `6A <group> 00`.
3. Wheel fields were decoded with `be16`; observed `17 00` is little-endian.

Fixes committed:
- `10_transport.ino`: ABS-only explicit-length request encoding.
- `30_modules.ino`: correct reply identifier and little-endian wheel speeds.

**Live-vehicle verification (2026-10-27):** After disconnecting the Autel and restoring the custom board, the ESP32 successfully read `2A 01 01`, `2A 01 02` and `2A 01 03` directly. When the session went idle, requests timed out; reconnecting and issuing them promptly yielded success. The firmware now cycles these groups every ~300 ms, with 3-failure session shutdown. New OTA release from commit `73efb4c7c9df387c12a7d2225c8c37a57d959c19` was installed successfully. The scanner reports `CONNECTED - ABS LIVE DATA`, `polling=1`, and valid live measurements: all four wheels 0 km/h, ABS supply 13.21 V, sensor monitor 3.43 V, brake switches released, pump stopped, front outlet valves not commanded. No ABS actuation or write service was transmitted.
