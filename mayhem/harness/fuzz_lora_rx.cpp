// gatewayLORA.cpp LORAtoX(): one packet received over the air (any LoRa transmitter on the configured
// frequency can send one). Byte 0 holds the gateway settings the packet is processed under
// (bit 0 onlyKnown, bit 1 Home Assistant discovery); the rest is the packet.
#include <stdint.h>
#include <stddef.h>

#include "stubs.h"
#include "TheengsCommon.h"
#include "config_LORA.h"

extern LORAConfig_s LORAConfig;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 1) return 0;
  mayhem_reset();
  LORAConfig.onlyKnown = data[0] & 1;
  mayhem_set_discovery(data[0] & 2);
  mayhem_lora_feed(data + 1, size - 1);
  LORAtoX();
  return 0;
}
