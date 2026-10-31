# JSON workspace cleanup — 2026-10-31

## Why the current evidence points to firmware work

After the user reported needing three or four manual resets, small read-only
checks of installed `1.10.26-history-stack-test` showed:

- Uptime 485,931 ms; automatic monitoring active and EGS connected.
- 657 successful SD history writes, zero read/write errors.
- 61,276 free heap bytes; 40,724 minimum free; 38,900 largest allocation.
- 1,312 minimum remaining loop-stack bytes.
- Five bounded HTTP send timeouts, increasing to eight during subsequent passive
  checks while engine/EGS visits and continuous uptime remained visible.
- Wi-Fi connected with approximately -50 dBm RSSI and one connection attempt.
- Latest reset reason `ESP_RST_POWERON`, consistent with the user's restart. This
  does not recover the cause of an earlier outage before a manual reset.

These observations do not demonstrate CPU saturation, SD throughput exhaustion,
or heap exhaustion. They also do not prove that stack overflow caused any outage.
The confirmed chart-transfer deadline failure remains documented separately in
`history_stack_isolation_2026-10-31.md`. It must not be presented as an explanation
for every whole-device reachability loss.

## Concrete stack use found in the installed build

Sensor HTTP responses can be served inside ECU response waits. The version26 ELF
contains this EGS DTC/environment call chain (individual fixed frames):

`readDTCs 608 → readEgsDtcEnvironment 352 → sendKwpPayload 688 →
readUntilEcuFrame 48 → WebServer dispatch → sensorsSend 144 →
JSON writer 1,488 → sensor serialization/formatting`.

These are frame sizes, not a measured complete call-stack total. Library parsing,
formatting, dispatch, and other parent frames also consume stack. Unlike the
completed history response, sensor serialization does not wait until outside a
diagnostic transaction. The JSON writer's 1,436-byte chunk buffer is therefore
material to the small observed stack margin.

## Replacement, without another transport or history path

Version27 requires an explicit `BoundedJson::Workspace` from its caller. The
encoder no longer owns a 1,436-byte array. Production provides exactly one fixed
workspace outside the emitter template, shared by the sequential counting and
output passes and by both production JSON endpoints. There is no compatibility
constructor, heap allocation, additional history store, or response-sized buffer.

This relies on the current synchronous single HTTP owner: sending cooperates
only through `delay(1)`, never `web.handleClient()`. Neither the serializer nor
its callback re-enters HTTP. A future concurrent HTTP implementation must supply
independent workspaces rather than sharing this one across simultaneous writers.

Response bytes, MSS chunk size, the 250 ms deadline, SD query semantics, UART
ownership, ECU requests, and all scheduling/timing settings are unchanged. This
cleanup is separate from changing the chart send deadline.

## Verification and deployment boundary

The full native/shared-client/mobile/Pi/advanced/source-contract checks pass.
AddressSanitizer and UndefinedBehaviorSanitizer checks pass for large JSON and
HTTP serialization. Tests verify exact Content-Length and bytes for responses
over 40 KB, timeout/abort behavior, temporary socket stalls, and a byte-identical
successful response after reusing the workspace from a failed response. An
encoder-size assertion prevents putting the chunk array back into each encoder.

The private SDK build passes with the actual target and OTA partition settings.
Comparing the installed version26 ELF with the version27 candidate:

| Compiled frame | Version26 | Version27 |
| --- | ---: | ---: |
| Sensor JSON writer |1,488 bytes |64 bytes |
| History JSON writer |1,504 bytes |80 bytes |
| Sensor handler |144 bytes |144 bytes |
| History handler |544 bytes |544 bytes |

Both writers remove1,424 bytes from the call stack. The linker contains exactly
one1,436-byte `httpJsonWorkspace` symbol. Reported static RAM increases from
122,864 to124,304 bytes including alignment. SDK program size is1,412,572 bytes
against the1,966,080-byte OTA partition. The actual image is1,412,720 bytes,
SHA-256 `87867c1299501695a977772a793e5935961b75ef1a9dfb5e8a61b76299d94dc1`.
The image and matching SD bundle remain private build artifacts.

These are compiled/native results, not measured version27 runtime stack margins.
The workspace candidate has not been installed; version26 remains live. No ECU
session, operating mode, SD journal or live firmware was changed during this
investigation. Before qualification, compare runtime stack headroom during both
engine and EGS DTC polling, verify normal saved-chart responses, and retain the
separate chart-timeout and scanner-reachability investigations. Physical Pi kiosk
qualification also remains incomplete.
