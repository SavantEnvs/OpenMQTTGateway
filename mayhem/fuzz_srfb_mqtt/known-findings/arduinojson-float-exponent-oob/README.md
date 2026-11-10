# ArduinoJson 6.18.5 indexes past its power-of-ten table for a long digit string

The MQTT payload is parsed the way `main.cpp` `receivingDATA()` does it, with `deserializeJson` (ArduinoJson 6.18.5),
and `XtoSRFB` reads `"value"`. A numeric string with 80 digits makes `FloatTraits::make_float` index `index 6` of a
`const float[6]` (UBSan: `FloatTraits.hpp:164`).

Reproduce: `/mayhem/fuzz_srfb_mqtt-standalone mayhem/fuzz_srfb_mqtt/known-findings/arduinojson-float-exponent-oob/input`

Impact: any MQTT publisher on the command topics hits a one-element out-of-bounds read in the JSON library
(third-party code compiled into the firmware).
Fix: update ArduinoJson past 6.18.5.
