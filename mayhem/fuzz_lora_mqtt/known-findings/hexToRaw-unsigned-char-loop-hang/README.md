# `_hexToRaw` never terminates for 256 or more bytes

`lib/TheengsUtils/TheengsUtils.cpp` `_hexToRaw(in, out, rawSize)` loops `for (unsigned char p = 0; p < rawSize; p++)`.
`p` wraps at 255, so for `rawSize >= 256` (a `hex` string of 512 characters or more, which also satisfies the
`strlen(in) == rawSize * 2` check) the loop never ends, and it keeps writing `out[p]` into the VLA that `XtoLORA`
sized at `strlen(hex) / 2`. `_rawToHex` has the same `unsigned char` counter.

Reproduce (hangs, run it under `timeout`): `timeout 10 /mayhem/fuzz_lora_mqtt-standalone mayhem/fuzz_lora_mqtt/known-findings/hexToRaw-unsigned-char-loop-hang/input`

Impact: one MQTT message to the LoRa command topic freezes the gateway loop (denial of service).
Fix: make the counters `int`, and cap the packet length at the 255 bytes a LoRa frame can carry.
