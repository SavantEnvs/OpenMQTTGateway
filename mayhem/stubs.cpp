// Host stand-ins for the parts of the firmware (main/main.cpp, Arduino core, hardware libraries) that the
// gateway modules under test link against. The gateway sources themselves are compiled unmodified.
#include <Arduino.h>
#include <ArduinoLog.h>
#include <LoRa.h>
#include <Preferences.h>
#include <SPI.h>
#include <TheengsUtils.h>

#include "TheengsCommon.h"
#include "config_LORA.h"
#include "config_SRFB.h"
#include "stubs.h"

unsigned long mayhem_now_ms = 0;
Logging Log;
SPIClass SPI;
LoRaClass LoRa;
HardwareSerial Serial;
Preferences preferences;

GatewayState gatewayState = WAITING_ONBOARDING;
SYSConfig_s SYSConfig = {};
bool ready_to_sleep = false;
char mqtt_topic[parameters_size + 1] = Base_Topic;
char gateway_name[parameters_size + 1] = Gateway_Name;
unsigned long lastDiscovery = 0;

namespace {
std::string g_published, g_serial_out, g_lora_out;
std::string g_serial_in, g_lora_in;
size_t g_serial_pos = 0, g_lora_pos = 0;
size_t g_lora_pkt = 0;
constexpr size_t kMaxSink = 1 << 16;

void sink(std::string& to, const std::string& what) {
  if (to.size() < kMaxSink) to += what;
}

struct ReceivedSignal {
  uint64_t value;
  uint32_t time;
};
ReceivedSignal receivedSignal[12];
#define struct_size (sizeof(receivedSignal) / sizeof(ReceivedSignal))
} // namespace

extern LORAConfig_s LORAConfig;
void LORAConfig_init();
extern std::vector<LORAdevice*> LORAdevices;
extern int newLORADevices;

extern unsigned char _uartbuf[RF_MESSAGE_SIZE + 3];
extern unsigned char _uartpos;

void mayhem_reset() {
  // Wall clock for the debounce statics in the modules: always later than anything a previous input left
  // behind, so an input's outcome never depends on how many inputs ran before it.
  static unsigned long clock_ms = 100000;
  clock_ms = (clock_ms + 1000) & 0xFFFFFFFFul;
  mayhem_now_ms = clock_ms;
  g_published.clear();
  g_serial_out.clear();
  g_lora_out.clear();
  g_serial_in.clear();
  g_lora_in.clear();
  g_serial_pos = g_lora_pos = g_lora_pkt = 0;
  memset(receivedSignal, 0, sizeof(receivedSignal));
  gatewayState = WAITING_ONBOARDING;
  SYSConfig = {};
  SYSConfig.mqtt = true;
  SYSConfig.discovery = true;
  strcpy(mqtt_topic, Base_Topic);
  strcpy(gateway_name, Gateway_Name);
  LORAConfig_init();
  for (auto d : LORAdevices) delete d;
  LORAdevices.clear();
  newLORADevices = 0;
  memset(_uartbuf, 0, sizeof(_uartbuf));
  _uartpos = 0;
}

void mayhem_serial_feed(const uint8_t* d, size_t n) { g_serial_in.assign((const char*)d, n); g_serial_pos = 0; }
void mayhem_lora_feed(const uint8_t* d, size_t n) { g_lora_in.assign((const char*)d, n); g_lora_pos = 0; g_lora_pkt = n > 255 ? 255 : n; }
const std::string& mayhem_published() { return g_published; }
const std::string& mayhem_serial_out() { return g_serial_out; }
const std::string& mayhem_lora_out() { return g_lora_out; }
void mayhem_set_discovery(bool on) { SYSConfig.discovery = on; }

int HardwareSerial::available() { return (int)(g_serial_in.size() - g_serial_pos); }
int HardwareSerial::read() { return g_serial_pos < g_serial_in.size() ? (uint8_t)g_serial_in[g_serial_pos++] : -1; }
int HardwareSerial::peek() { return g_serial_pos < g_serial_in.size() ? (uint8_t)g_serial_in[g_serial_pos] : -1; }
size_t HardwareSerial::write(uint8_t c) { sink(g_serial_out, std::string(1, (char)c)); return 1; }

int LoRaClass::parsePacket(int) { return (int)g_lora_pkt; }
int LoRaClass::read() { return g_lora_pos < g_lora_pkt ? (uint8_t)g_lora_in[g_lora_pos++] : -1; }
int LoRaClass::available() { return (int)(g_lora_pkt - g_lora_pos); }
size_t LoRaClass::write(const uint8_t* b, size_t n) { sink(g_lora_out, std::string((const char*)b, n)); return n; }
size_t LoRaClass::print(const char* c) { if (c) sink(g_lora_out, c); return c ? strlen(c) : 0; }

// main.cpp: enqueueJsonObject() serializes the document before queueing it.
bool enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc, int) {
  if (jsonDoc.size() == 0) {
    gatewayState = GatewayState::ERROR;
    return true;
  }
  std::string s;
  serializeJson(jsonDoc, s);
  sink(g_published, s + "\n");
  return true;
}
bool enqueueJsonObject(const StaticJsonDocument<JSON_MSG_BUFFER>& jsonDoc) { return enqueueJsonObject(jsonDoc, 0); }

// main.cpp, verbatim apart from the BT-only branches.
void buildTopicFromId(JsonObject& Jsondata, const char* origin) {
  if (!Jsondata.containsKey("id")) {
    gatewayState = GatewayState::ERROR;
    return;
  }
  std::string topic = Jsondata["id"].as<std::string>();
  size_t pos = topic.find(":");
  while (pos != std::string::npos) {
    topic.erase(pos, 1);
    pos = topic.find(":", pos);
  }
  std::string subjectStr(origin);
  topic = subjectStr + "/" + topic;
  Jsondata["origin"] = topic;
}

// main.cpp, verbatim.
bool cmpToMainTopic(const char* topicOri, const char* toAdd) {
  if (strcmp(topicOri, toAdd) == 0)
    return true;
  if (strncmp(topicOri, mqtt_topic, strlen(mqtt_topic)) != 0)
    return false;
  topicOri += strlen(mqtt_topic);
  if (strncmp(topicOri, gateway_name, strlen(gateway_name)) != 0)
    return false;
  topicOri += strlen(gateway_name);
  if (strncmp(topicOri, toAdd, strlen(toAdd)) != 0)
    return false;
  return true;
}

// main.cpp duplicate-signal bookkeeping, verbatim.
int getMin() {
  unsigned int minimum = receivedSignal[0].time;
  int minindex = 0;
  for (int i = 1; i < struct_size; i++) {
    if (receivedSignal[i].time < minimum) {
      minimum = receivedSignal[i].time;
      minindex = i;
    }
  }
  return minindex;
}

void storeSignalValue(uint64_t MQTTvalue) {
  unsigned long now = millis();
  int o = getMin();
  receivedSignal[o].value = MQTTvalue;
  receivedSignal[o].time = now;
}

bool isAduplicateSignal(uint64_t value) {
  for (int i = 0; i < struct_size; i++) {
    if (receivedSignal[i].value == value) {
      unsigned long now = millis();
      if (now - receivedSignal[i].time < time_avoid_duplicate)
        return true;
    }
  }
  return false;
}

bool pub(const char* topic, const char* payload, bool) { sink(g_published, std::string(topic) + " " + payload + "\n"); return true; }
bool pub(const char* topic, const char* payload) { return pub(topic, payload, false); }
unsigned long uptime() { return millis() / 1000; }

// Home Assistant discovery publishing: only the call is observable here.
void createDiscovery(const char* sensor_type, const char* st_topic, const char* s_name, const char* unique_id,
                     const char*, const char*, const char*, const char*, const char*, const char*, int,
                     const char*, const char*, bool, const char*, const char*, const char*, const char*,
                     const char*, bool, const char*, const char*, const char*, const char*, const char*, bool) {
  sink(g_published, std::string("discovery ") + (sensor_type ? sensor_type : "") + " " + (st_topic ? st_topic : "") + " " +
                        (s_name ? s_name : "") + " " + (unique_id ? unique_id : "") + "\n");
}
