#pragma once
#include <stdint.h>

class IPAddress {
public:
  IPAddress() : v_{0, 0, 0, 0} {}
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : v_{a, b, c, d} {}
  uint8_t operator[](int i) const { return v_[i & 3]; }
  uint8_t& operator[](int i) { return v_[i & 3]; }
private:
  uint8_t v_[4];
};
