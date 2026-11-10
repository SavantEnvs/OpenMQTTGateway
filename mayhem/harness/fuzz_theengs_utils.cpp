// TheengsUtils (lib/TheengsUtils): the hex/string helpers the gateway modules apply to over-the-air and
// MQTT-supplied data. Byte 0 selects the function, the rest is its argument.
#include <Arduino.h>
#include <IPAddress.h>
#include <TheengsUtils.h>

#include <string>
#include <vector>

#include "stubs.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 1) return 0;
  const uint8_t sel = data[0] % 8;
  const uint8_t* p = data + 1;
  const size_t n = size - 1;
  const std::string text((const char*)p, n); // NUL-terminated by std::string

  switch (sel) {
    case 0: {
      uint64_t v = 0;
      memcpy(&v, p, n < 8 ? n : 8);
      String s = TheengsUtils::toString(v);
      // known-answer: decimal rendering must match printf's
      char ref[32];
      snprintf(ref, sizeof ref, "%llu", (unsigned long long)v);
      if (strcmp(s.c_str(), ref) != 0) abort();
      break;
    }
    case 1:
      TheengsUtils::processCert(text.c_str());
      break;
    case 2: {
      std::string h = TheengsUtils::generateHash(text);
      if (h.size() != 64) abort();
      break;
    }
    case 3: {
      // value_from_hex_data(service_data, offset, data_length, ...) extracts data_length characters at offset
      // of a hex string: offset/length are drawn from the input and kept inside the string.
      if (n < 3) return 0;
      const uint8_t flags = p[0];
      const std::string hex((const char*)p + 3, n - 3);
      const size_t off = p[1] % (hex.size() + 1);
      const size_t len = p[2] % (hex.size() - off + 1);
      TheengsUtils::value_from_hex_data(hex.c_str(), (int)off, (int)len, flags & 1, flags & 2);
      break;
    }
    case 4: {
      // _rawToHex writes 2 * size + 2 bytes (see gatewayLORA.cpp); the buffer is sized to match for any n, so a
      // size of 256 or more reaches the library's unsigned-char loop counter (a hang, a finding).
      std::vector<char> hex(n * 2 + 2);
      std::vector<byte> in(p, p + n), back(n ? n : 1);
      TheengsUtils::_rawToHex(in.data(), hex.data(), (int)n);
      hex[n * 2] = 0;
      if (!TheengsUtils::_hexToRaw(hex.data(), back.data(), (int)n)) abort();
      if (n && memcmp(in.data(), back.data(), n) != 0) abort();
      break;
    }
    case 5: {
      if (n < 2) return 0;
      const int raw = p[0] | (p[1] << 8);
      std::vector<byte> out(raw ? raw : 1);
      TheengsUtils::_hexToRaw(std::string((const char*)p + 2, n - 2).c_str(), out.data(), raw);
      break;
    }
    case 6: {
      IPAddress ip(n > 0 ? p[0] : 0, n > 1 ? p[1] : 0, n > 2 ? p[2] : 0, n > 3 ? p[3] : 0);
      char* s = TheengsUtils::ip2CharArray(ip);
      if (!s) abort();
      TheengsUtils::to_bool(String(text.c_str()));
      break;
    }
    case 7: {
      float f = 0;
      memcpy(&f, p, n < 4 ? n : 4);
      TheengsUtils::round2(f);
      break;
    }
  }
  return 0;
}
