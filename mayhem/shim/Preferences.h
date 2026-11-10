#pragma once
#include "Arduino.h"

class Preferences {
public:
  bool begin(const char*, bool = false) { return true; }
  void end() {}
  bool isKey(const char*) { return false; }
  size_t remove(const char*) { return 1; }
  size_t putString(const char*, const String& v) { return v.length(); }
  String getString(const char*, const String& def = String()) { return def; }
};
