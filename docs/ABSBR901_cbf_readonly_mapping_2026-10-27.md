# ABSBR901 KWFB / read-only measurements (CBF verified)

Date: 2026-10-27. Source: user-supplied `ABSBR901(2).cbf` (176340 bytes, SHA256 `3c9518b96ec61d6b43373147c7d13902b511214d6340cc94049ab4c303aa35fa`).

**Status updated 2026-10-27:** Autel and the ESP32 both successfully initiated the ABS diagnostic session and exchanged the CBF-defined read-only groups 01/02/03. ESP32 automatic polling was installed and validated. This document lists diagnostic payloads; full wire frames are in the verified capture report. Output controls, coding, fault clearing, and unverified groups remain disabled.

## Communication interface

- Interface: `KWFB`, version `01.00.07`; `KLINE_KWFB_10_4`.
- CBF initialization communication parameters: `CP_TRIGADDRESS = 0x34`, `CP_KEYWORD1 = 0xC2`, `CP_KEYWORD2 = 0x46`.
- A prior ESP32 KWP StartCommunication attempt yielded `83 F3 34 C1 43 46 F4`. The first returned key `0x43` **differs** from CBF's configured `0xC2`. A successful Autel session also received `43 46` and then read all three groups. Thus the first keyword mismatch **was not** the reason ABS read polling had failed.
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

CBF response bit positions are **relative to the start of the diagnostic response service payload**. Actual response prefix `6A 01 00` was captured from Autel and then from the ESP32; see verified capture report.

| Location | CBF field | Width | Unit |
| --- | --- | --- | --- |
| byte 3–4 (bit 24) | `DT_Rad_V_HL` rear left | 2 bytes | km/h |
| byte 5–6 (bit 40) | `DT_Rad_V_VR` front right | 2 bytes | km/h |
| byte 7–8 (bit 56) | `DT_Rad_V_HR` rear right | 2 bytes | km/h |
| byte 9–10 (bit 72) | `DT_Rad_V_VL` front left | 2 bytes | km/h |

CBF presentation `PRES_Radgeschwindigkeit`: 16 bits, `ByteOrder=1`; raw 0..38 -> 0 km/h, raw 39..4800 -> raw * 0.0625 km/h. **Little-endian wheel values were confirmed** by the captured `17 00` readings (23 counts, below the 0 km/h threshold). The group-wide `DT_Radgeschwindigkeiten` dump covers all eight bytes at bit 24.

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

## Deployment guardrails and verified running state

- Do not send `31` routine-control, `27` security or variant-coding writes, brake actuation, or clear-DTC services.
- Working ABS-only request frame: `80 34 F3 03 2A 01 01 D6`; working response: `8B F3 34 6A 01 00 17 00 17 00 17 00 17 00 79`.
- Firmware release commit `73efb4c7c9df387c12a7d2225c8c37a57d959c19` installed via OTA, 1,310,560 bytes.
- ABS read-only automatic polling enabled for groups 01/02/03, every ~300 ms between group requests. Tested against the live module at `192.168.8.168` with all 11 dashboard values valid. Status `CONNECTED - ABS LIVE DATA`.
- Preserve the explicit-length framing and little-endian wheel-speed decoding in future changes. The engine, transmission and other modules retain their existing transport behavior.
