// Minimal host-side Arduino/ESP32 API surface, just enough to compile OpenMQTTGateway's
// own gateway sources unmodified under clang with libFuzzer. Not a hardware model.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <string>
#include <vector>

typedef uint8_t byte;
typedef bool boolean;
typedef uint32_t TickType_t;
typedef int BaseType_t;

class __FlashStringHelper;
#define F(s)           (reinterpret_cast<const __FlashStringHelper*>(s))
#define PSTR(s)        (s)
#define PROGMEM
#define sprintf_P      sprintf
#define snprintf_P     snprintf
#define strlen_P       strlen
#define pgm_read_byte(a) (*(const uint8_t*)(a))

#define HIGH   1
#define LOW    0
#define INPUT  0
#define OUTPUT 1

using std::max;
using std::min;

extern unsigned long mayhem_now_ms;
inline unsigned long millis() { return mayhem_now_ms; }
inline void delay(unsigned long ms) { mayhem_now_ms += ms; }
inline void yield() {}
inline void configTime(long, int, const char*) {}
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline int xPortGetCoreID() { return 0; }

#include "WString.h"
#include "Print.h"

class HardwareSerial : public Stream {
public:
  void begin(unsigned long) {}
  int available() override;
  int read() override;
  int peek() override;
  size_t write(uint8_t c) override;
  using Print::write;
};
extern HardwareSerial Serial;

// FreeRTOS semaphores: a single-threaded host never blocks.
typedef void* SemaphoreHandle_t;
#define pdMS_TO_TICKS(x) (x)
#define pdTRUE           1
#define pdFALSE          0
inline SemaphoreHandle_t xSemaphoreCreateBinary() { return (SemaphoreHandle_t)1; }
inline int xSemaphoreTake(SemaphoreHandle_t, TickType_t) { return pdTRUE; }
inline int xSemaphoreGive(SemaphoreHandle_t) { return pdTRUE; }
