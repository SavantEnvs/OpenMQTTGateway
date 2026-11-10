# `_rfbDecode` overflows `buffer` for every RFIN frame

`main/gatewaySRFB.cpp` `_rfbDecode()` declares `char buffer[RF_MESSAGE_SIZE * 2 + 1]` (19 bytes) and passes it to
`TheengsUtils::_rawToHex(&_uartbuf[1], buffer, RF_MESSAGE_SIZE)`, which does `sprintf("%02X\r")` per byte: three
characters per byte, `3 * 9 + 1 = 28` bytes written (ASan: `stack-buffer-overflow` in `vsprintf`).
The input is the serial stream `AA A4` and the harness closes the frame with `55`; any received RFIN frame, that is
any 433 MHz code the bridge receives, triggers it.

Reproduce: `/mayhem/fuzz_srfb_uart-standalone mayhem/fuzz_srfb_uart/known-findings/rfbdecode-stack-overflow/input`

Impact: attacker-controlled bytes (the received code) are written past a stack buffer.
Fix: size `buffer` at `RF_MESSAGE_SIZE * 3 + 1` or make `_rawToHex` emit `%02X` without the `\r`.

Coverage ceiling: `_rfbDecode` acts only on `RF_CODE_RFIN`; every other action byte (ack, RFOUT, learn, bucket,
advanced RF) returns immediately, so those frames add no branches. The only frame that reaches code past the
first line of `_rfbDecode` is RFIN, and it aborts in `_rawToHex` before `value_from_hex_data`, the JSON build and
`_rfbAck` run. `fuzz_srfb_uart` therefore saturates at the ~60 edges of `SRFBtoX` and the head of `_rfbDecode`;
the rest of the decoder only becomes reachable once the overflow is fixed upstream.
