# `XtoLORA` makes a zero-length VLA from an empty `hex`

`main/gatewayLORA.cpp` `XtoLORA()`: `byte raw[strlen(hex) / 2];` is zero for `"hex":""` or a one-character string
(UBSan: `variable length array bound evaluates to non-positive value 0`); `_hexToRaw` then rejects the odd/empty
string and the uninitialised `raw` is still handed to `LoRa.write`.

Reproduce: `/mayhem/fuzz_lora_mqtt-standalone mayhem/fuzz_lora_mqtt/known-findings/hex-zero-length-vla/input`

Impact: undefined behaviour from an MQTT command; for odd lengths up to 511 characters the radio transmits
uninitialised stack bytes.
Fix: reject `strlen(hex)` that is zero or odd before declaring `raw`.
