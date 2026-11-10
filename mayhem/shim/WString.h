#pragma once
// Arduino String semantics (bounds-tolerant, like the ESP32 core's WString) over std::string.
#include <string>
#include <stdlib.h>
#include <string.h>

class __FlashStringHelper;

class String {
public:
  String() {}
  String(const char* c) { if (c) s_ = c; }
  String(const __FlashStringHelper* f) { if (f) s_ = reinterpret_cast<const char*>(f); }
  String(const String& o) : s_(o.s_) {}
  String(const std::string& o) : s_(o) {}
  String(char c) : s_(1, c) {}
  String(int v, unsigned char base = 10) { init_num((long long)v, base); }
  String(unsigned v, unsigned char base = 10) { init_unum(v, base); }
  String(long v, unsigned char base = 10) { init_num(v, base); }
  String(unsigned long v, unsigned char base = 10) { init_unum(v, base); }
  String(long long v, unsigned char base = 10) { init_num(v, base); }
  String(unsigned long long v, unsigned char base = 10) { init_unum(v, base); }
  String(float v, unsigned char d = 2) { init_dbl(v, d); }
  String(double v, unsigned char d = 2) { init_dbl(v, d); }

  String& operator=(const String& o) { s_ = o.s_; return *this; }
  String& operator=(const char* c) { s_ = c ? c : ""; return *this; }
  const char* c_str() const { return s_.c_str(); }
  unsigned int length() const { return (unsigned int)s_.size(); }
  bool isEmpty() const { return s_.empty(); }
  bool reserve(unsigned int n) { s_.reserve(n); return true; }
  char operator[](unsigned int i) const { return i < s_.size() ? s_[i] : 0; }
  char charAt(unsigned int i) const { return (*this)[i]; }

  bool concat(const String& o) { s_ += o.s_; return true; }
  bool concat(const char* c) { if (c) s_ += c; return true; }
  bool concat(const char* c, unsigned int n) { if (c) s_.append(c, n); return true; }
  bool concat(char c) { s_ += c; return true; }
  template <typename T> bool concat(T v) { return concat(String(v)); }
  String& operator+=(const String& o) { concat(o); return *this; }
  String& operator+=(const char* c) { concat(c); return *this; }
  String& operator+=(char c) { concat(c); return *this; }
  template <typename T> String& operator+=(T v) { concat(String(v)); return *this; }

  friend String operator+(const String& a, const String& b) { String r(a); r.concat(b); return r; }
  friend String operator+(const String& a, const char* b) { String r(a); r.concat(b); return r; }
  friend String operator+(const char* a, const String& b) { String r(a); r.concat(b); return r; }
  friend String operator+(const String& a, char b) { String r(a); r.concat(b); return r; }
  friend String operator+(char a, const String& b) { String r(a); r.concat(b); return r; }
  template <typename T> friend String operator+(const String& a, T b) { String r(a); r.concat(String(b)); return r; }

  bool operator==(const String& o) const { return s_ == o.s_; }
  bool operator==(const char* c) const { return s_ == (c ? c : ""); }
  bool operator!=(const String& o) const { return s_ != o.s_; }
  bool operator!=(const char* c) const { return s_ != (c ? c : ""); }
  bool operator<(const String& o) const { return s_ < o.s_; }
  bool equals(const String& o) const { return s_ == o.s_; }
  bool equals(const char* c) const { return *this == c; }

  bool startsWith(const String& p) const { return s_.compare(0, p.s_.size(), p.s_) == 0 && s_.size() >= p.s_.size(); }
  bool endsWith(const String& p) const {
    return s_.size() >= p.s_.size() && s_.compare(s_.size() - p.s_.size(), p.s_.size(), p.s_) == 0;
  }
  int indexOf(char c, unsigned int from = 0) const { size_t p = s_.find(c, from); return p == std::string::npos ? -1 : (int)p; }
  int indexOf(const String& o, unsigned int from = 0) const { size_t p = s_.find(o.s_, from); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(char c) const { size_t p = s_.rfind(c); return p == std::string::npos ? -1 : (int)p; }
  int lastIndexOf(const String& o) const { size_t p = s_.rfind(o.s_); return p == std::string::npos ? -1 : (int)p; }
  String substring(unsigned int from) const { return substring(from, length()); }
  String substring(unsigned int from, unsigned int to) const {
    if (from > to) { unsigned int t = from; from = to; to = t; }
    if (from >= s_.size()) return String();
    if (to > s_.size()) to = (unsigned int)s_.size();
    return String(s_.substr(from, to - from));
  }
  void remove(unsigned int idx) { if (idx < s_.size()) s_.erase(idx); }
  void remove(unsigned int idx, unsigned int count) { if (idx < s_.size() && count) s_.erase(idx, count); }
  void replace(const String& find, const String& rep) {
    if (find.s_.empty()) return;
    size_t p = 0;
    while ((p = s_.find(find.s_, p)) != std::string::npos) { s_.replace(p, find.s_.size(), rep.s_); p += rep.s_.size(); }
  }
  void toUpperCase() { for (auto& c : s_) c = (char)toupper((unsigned char)c); }
  void toLowerCase() { for (auto& c : s_) c = (char)tolower((unsigned char)c); }
  void trim() {
    size_t b = s_.find_first_not_of(" \t\r\n"), e = s_.find_last_not_of(" \t\r\n");
    s_ = b == std::string::npos ? "" : s_.substr(b, e - b + 1);
  }
  long toInt() const { return atol(s_.c_str()); }
  float toFloat() const { return (float)atof(s_.c_str()); }
  double toDouble() const { return atof(s_.c_str()); }
  const std::string& str() const { return s_; }

private:
  void init_num(long long v, unsigned char base) {
    char b[80];
    if (base == 16) snprintf(b, sizeof b, "%llx", (unsigned long long)v); else snprintf(b, sizeof b, "%lld", v);
    s_ = b;
  }
  void init_unum(unsigned long long v, unsigned char base) {
    char b[80];
    if (base == 16) snprintf(b, sizeof b, "%llx", v); else snprintf(b, sizeof b, "%llu", v);
    s_ = b;
  }
  void init_dbl(double v, unsigned char d) { char b[400]; snprintf(b, sizeof b, "%.*f", d, v); s_ = b; }
  std::string s_;
};

class StringSumHelper : public String {};
