# `LORAtoX` keeps using its `JsonObject` after the packet parsed as a JSON scalar

`main/gatewayLORA.cpp` `LORAtoX()` takes `JsonObject LORAdata = LORAdataBuffer.to<JsonObject>()`, then for an ASCII
packet calls `deserializeJson(LORAdataBuffer, packet)`. A packet that is valid JSON but not an object (a bare number,
`true`, an array) replaces the document root while `LORAdata` still treats it as an object; the next
`LORAdata["rssi"] = ...` walks the wrong structure (UBSan: member call on misaligned `VariantSlot` address 0x2 for
the packet `2`; ASan SEGV in `VariantSlot::key()` for some arrays).

Reproduce: `/mayhem/fuzz_lora_rx-standalone mayhem/fuzz_lora_rx/known-findings/json-scalar-packet-type-confusion/input`
(byte 0 is the config flags, the rest is the LoRa packet: `2`).

Impact: any LoRa transmitter in range can crash the gateway with a one-character packet.
Fix: after a successful `deserializeJson` re-take `LORAdata = LORAdataBuffer.as<JsonObject>()` and bail out if it is null.
