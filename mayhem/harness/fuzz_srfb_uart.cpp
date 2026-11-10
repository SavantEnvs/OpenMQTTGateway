// gatewaySRFB.cpp SRFBtoX(): the Sonoff RF Bridge's serial stream, i.e. what a 433 MHz transmitter in range of
// the bridge makes it send to the ESP: AA <action> <payload> 55 frames, decoded into MQTT JSON.
#include <stdint.h>
#include <stddef.h>

#include "stubs.h"

extern bool SRFBtoX();

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  mayhem_reset();
  mayhem_serial_feed(data, size);
  bool receiving = SRFBtoX();
  // SRFBtoX keeps a frame-in-progress in a function static; close it so the next input starts clean.
  const uint8_t stop = 0x55;
  if (receiving) {
    mayhem_serial_feed(&stop, 1);
    SRFBtoX();
  }
  return 0;
}
