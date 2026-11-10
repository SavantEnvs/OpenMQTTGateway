# `revert_hex_data` reads two bytes before its input buffer

`lib/TheengsUtils/TheengsUtils.cpp` `revert_hex_data(in, out, l)` loops `while (i != -2)` starting at `i = l - 2`.
`value_from_hex_data(..., reverse=true, ...)` calls it with `l = data_length + 1`, an odd number for the usual even
field length, so `i` walks down through odd numbers to `-1`; `-1 % 2 != 0` takes the `in[i - 1]` branch and reads
`in[-2]`, two bytes below the `data[]` stack array, on every reversed field (ASan: `dynamic-stack-buffer-overflow`).

Reproduce: `/mayhem/fuzz_theengs_utils-standalone mayhem/fuzz_theengs_utils/known-findings/revert-hex-data-oob-read/input`
(byte 0 selects `value_from_hex_data`, byte 1 bit 0 is `reverse`).

Impact: stack over-read in every BLE decode that reverses bytes; stack garbage can reach the decoded value.
Fix: loop `while (i >= 0)` and write `out[l - 1] = '\0'` (iterate `l - 1` times).
