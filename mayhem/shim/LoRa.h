#pragma once
// Stand-in for the sandeepmistry/LoRa driver: the radio side is scripted by the harness through
// lora_feed(); transmit calls are recorded so the harness can check them.
#include "Arduino.h"

void lora_feed(const uint8_t* data, size_t n);

class LoRaClass {
public:
  bool begin(long) { return true; }
  void setPins(int, int, int) {}
  void receive() {}
  void setFrequency(long) {}
  void setTxPower(int) {}
  void setSpreadingFactor(int) {}
  void setSignalBandwidth(long) {}
  void setCodingRate4(int) {}
  void setPreambleLength(long) {}
  void setSyncWord(int) {}
  void enableCrc() {}
  void disableCrc() {}
  void enableInvertIQ() {}
  void disableInvertIQ() {}
  int parsePacket(int = 0);
  int read();
  int available();
  float packetRssi() { return -80; }
  float packetSnr() { return 5; }
  float packetFrequencyError() { return 0; }
  int beginPacket(int = 0) { return 1; }
  int endPacket(bool = false) { return 1; }
  size_t write(const uint8_t* b, size_t n);
  size_t print(const char* c);
};
extern LoRaClass LoRa;
