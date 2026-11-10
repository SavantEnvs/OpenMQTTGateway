#pragma once
#include "WString.h"
#include <stddef.h>
#include <stdint.h>

class Print;
class Printable {
public:
  virtual ~Printable() {}
  virtual size_t printTo(Print& p) const = 0;
};

class Print {
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t) = 0;
  virtual size_t write(const uint8_t* b, size_t n) { size_t k = 0; while (n--) k += write(*b++); return k; }
  size_t print(const char* c) { return c ? write((const uint8_t*)c, strlen(c)) : 0; }
  size_t print(const String& s) { return write((const uint8_t*)s.c_str(), s.length()); }
  size_t println() { return write((uint8_t)'\n'); }
  size_t println(const char* c) { size_t n = print(c); n += write('\n'); return n; }
  virtual void flush() {}
};

class Stream : public Print {
public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  size_t readBytes(char* b, size_t n) { size_t k = 0; while (k < n && available()) b[k++] = (char)read(); return k; }
};
