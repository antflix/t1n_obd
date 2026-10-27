# ABSBR901 KWFB / read-only measurements (CBF verified)

Date: 2026-10-27. Source: user-supplied `ABSBR901(2).cbf` (176340 bytes, SHA256 `3c9518b96ec61d6b43373147c7d13902b511214d6340cc94049ab4c303aa35fa`).

**Status:** CBF request payloads and presentation metadata are decoded. Correct KWFB **wire/session handshake is NOT verified**. These are diagnostic payloads, NOT necessarily complete K-line frames. Do not enable ABS auto-poll or output-control services based on this document alone.

## Communication interface

- Interface: `KWFB`, version `01.00.07`; `KLINE_KWFB_10_4`.
- CBF initialization communication parameters: `CP_TRIGADDRESS = 0x34`, `CP_KEYWORD1 = 0xC2`, `CP_KEYWORD2 = 0x46`.
- A prior ESP32 KWP StartCommunication attempt yielded `83 F3 34 C1 43 46 F4`. The first returned key `0x43` **differs** from CBF's configured `0xC2`. A valid C1 response proves physical reachability, but not completion of the CBF's intended KWFB initialization. A distinct slow-address trigger/keyword handshake is a hypothesis to test against a known-good trace, not a confirmed solution.
- CBF defines the communication parameter keys `CP_RESPONSEMASTER`, `CP_REQTARGETBYTE`, `CP_RESPSOURCEBYTE`, `CP_TRIGADDRESS`, `CP_KEYWORD1`, `CP_KEYWORD2`. Only three overrides above are present in the initialization service; defaults of the other three are not recovered.

## Read-only requests, exact CBF diagnostic payloads

| Request payload | Diagnostic group |
| --- | --- |
| `2A 01 01` | Wheel speeds |
| `2A 01 02` | Analog voltage/sensor inputs |
| `2A 01 03` | Brake switches, terminal input and valve feedback |
| `2A 01 04` | Engine RPM, accelerator, torque via ABS where applicable |
| `3C 02` | Production data |

All five payloads were extracted from the DiagService RequestBytes fields, not inferred from names. The CBF stores `2A`, `01`, group number as three constant request preparations (`Mode_Byte`, `Wiederholrate`, and group ID).

## Wheel group 2A 01 01 (response *service payload*, not K-line frame)

CBF response bit positions are **relative to the start of the diagnostic response service payload**. A response with prefix `6A 01 01` is expected by KWP convention but **has not been captured from this ABS ECU**.

| Location | CBF field | Width | Unit |
| --- | --- | --- | --- |
| byte 3–4 (bit 24) | `DT_Rad_V_HL` rear left | 2 bytes | km/h |
| byte 5–6 (bit 40) | `DT_Rad_V_VR` front right | 2 bytes | km/h |
| byte 7–8 (bit 56) | `DT_Rad_V_HR` rear right | 2 bytes | km/h |
| byte 9–10 (bit 72) | `DT_Rad_V_VL` front left | 2 bytes | km/h |

CBF presentation `PRES_Radgeschwindigkeit`: 16 bits, `ByteOrder=1`; raw 0..38 -> 0 km/h, raw 39..4800 -> raw * 0.0625 km/h. **Byte-order interpretation requires a real response to confirm**, despite the recorded code. The group-wide `DT_Radgeschwindigkeiten` dump covers all eight bytes at bit 24.

## Selected group 2A 01 02

- `DT_AE_UZ_KL15_KL87`: byte 3, one byte * 0.071 V.
- `DT_AE_SFA`: byte 4, one byte * 0.0196 (see CBF units).
- Other analog fields vary by variant; use exported variant-specific field catalog.

## Selected group 2A 01 03

- `DT_DE_BLS` brake-light switch: byte 3, `0x00` = not pressed, `0xFF` = pressed.
- `DT_DE_BS` brake switch: byte 4, `0x00` = pressed, `0xFF` = not pressed.
- The valve feedback, motor, and additional terminal/inputs have **per-field byte** definitions and variant-specific layouts (not just assumed bit flags). Refer to the catalog.

## Variants

The CBF includes base `ABSBR901` plus `ABS_ASR_5_3`, `ABSR_5_7_KM20`, `ABSR_5_7_KM20_ASG`, `ABSR_5_7_KM6`. Different variants reuse request groups with **different response field layouts**, particularly analog, digital and engine data. Do not combine mappings from all variants without first identifying the actual ECU variant.

## Capture audit

The archived `auto_scan.sr`, `sprinter_capture.sr`, `sprinter_capture(1).sr`, `sprinter_capture(2).sr` and `full_run.sr` were decoded offline as inverted UART at 10400 baud. The decodable channels contained other Mercedes-module traffic but no confirmed successful ABS request/response exchange to validate KWFB framing, byte order or timing.

## Deployment guardrails

- Do not send `31` routine-control, `27` security or variant-coding writes, brake actuation, or clear-DTC services.
- ABS auto-poll remains disabled.
- Next test: obtain a **known-good KWFB initiation and a single read-only reply**; correlate its key-byte sequence against `0xC2 0x46`, including whether a 5-baud `0x34` trigger is used. Only after that should firmware implement the transport and validate the above decoders.
