// Known-answer tests for the gateway code the fuzz targets drive: each case feeds a fixed input to a real
// firmware entry point and compares what the firmware publishes or transmits with the expected bytes.
#include "TheengsCommon.h" // first: its ARDUINOJSON_* macros must match the firmware build
#include <TheengsUtils.h>

#include <string>

#include "stubs.h"
#include "config_LORA.h"

extern bool SRFBtoX();
extern void XtoSRFB(const char*, const char*);
extern void XtoSRFB(const char*, JsonObject&);
extern void LORAtoX();
extern LORAConfig_s LORAConfig;

static int passed = 0, failed = 0;

static void check(const char* name, bool ok) {
  printf("%s %s\n", ok ? "PASS" : "FAIL", name);
  ok ? passed++ : failed++;
}

static void expect_str(const char* name, const std::string& got, const std::string& want) {
  bool ok = got == want;
  check(name, ok);
  if (!ok) printf("  got:  [%s]\n  want: [%s]\n", got.c_str(), want.c_str());
}

static std::string bytes(std::initializer_list<int> l) {
  std::string s;
  for (int b : l) s += (char)b;
  return s;
}

static void feed_lora(const std::string& s) { mayhem_lora_feed((const uint8_t*)s.data(), s.size()); }
static void feed_serial(const std::string& s) { mayhem_serial_feed((const uint8_t*)s.data(), s.size()); }

int main() {
  // ---- TheengsUtils
  expect_str("toString(0)", TheengsUtils::toString(0).c_str(), "0");
  expect_str("toString(max uint64)", TheengsUtils::toString(18446744073709551615ULL).c_str(), "18446744073709551615");
  check("value_from_hex_data 0x1234", TheengsUtils::value_from_hex_data("1234", 0, 4, false, false) == 0x1234);
  check("value_from_hex_data offset", TheengsUtils::value_from_hex_data("00FF00", 2, 2, false, false) == 0xFF);
  check("value_from_hex_data negative", TheengsUtils::value_from_hex_data("FFFE", 0, 4, false, true) == -1);
  expect_str("generateHash(abc)", TheengsUtils::generateHash("abc"),
             "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
  {
    byte raw[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    char hex[10] = {0};
    TheengsUtils::_rawToHex(raw, hex, 4);
    hex[8] = 0;
    expect_str("_rawToHex", hex, "DEADBEEF");
    byte back[4] = {0};
    check("_hexToRaw ok", TheengsUtils::_hexToRaw("DEADBEEF", back, 4) && memcmp(raw, back, 4) == 0);
    check("_hexToRaw wrong length", !TheengsUtils::_hexToRaw("DEADBE", back, 4));
  }
  check("to_bool(false)", !TheengsUtils::to_bool(String("false")));
  check("to_bool(1)", TheengsUtils::to_bool(String("1")));

  // ---- Sonoff RF Bridge: MQTT -> serial frame to the bridge
  mayhem_reset();
  XtoSRFB("/commands/MQTTtoSRFB", "123456");
  expect_str("SRFB plain MQTT -> RFOUT frame", mayhem_serial_out(),
             bytes({'\n', 0xAA, 0xA5, 0x25, 0x1C, 0x01, 0x40, 0x03, 0x84, 0x01, 0xE2, 0x40, 0x55, '\n'}));
  expect_str("SRFB plain MQTT ack", mayhem_published(), "/SRFBtoMQTT 123456\n");

  mayhem_reset();
  {
    StaticJsonDocument<JSON_MSG_BUFFER_MAX> doc;
    deserializeJson(doc, "{\"value\":123456,\"delay\":9500,\"val_Tlow\":320,\"val_Thigh\":900,\"repeat\":2}");
    JsonObject o = doc.as<JsonObject>();
    XtoSRFB("home/OpenMQTTGateway/commands/MQTTtoSRFB", o);
  }
  {
    std::string one = bytes({'\n', 0xAA, 0xA5, 0x25, 0x1C, 0x01, 0x40, 0x03, 0x84, 0x01, 0xE2, 0x40, 0x55, '\n'});
    expect_str("SRFB json repeat=2 -> two RFOUT frames", mayhem_serial_out(), one + one);
  }
  check("SRFB json ack origin", mayhem_published().find("\"origin\":\"/SRFBtoMQTT\"") != std::string::npos);

  mayhem_reset();
  {
    StaticJsonDocument<JSON_MSG_BUFFER_MAX> doc;
    deserializeJson(doc, "{\"raw\":\"251C0140038401E240\"}");
    JsonObject o = doc.as<JsonObject>();
    XtoSRFB("/commands/MQTTtoSRFB", o);
  }
  expect_str("SRFB json raw -> RFOUT frame", mayhem_serial_out(),
             bytes({'\n', 0xAA, 0xA5, 0x25, 0x1C, 0x01, 0x40, 0x03, 0x84, 0x01, 0xE2, 0x40, 0x55, '\n'}));

  // ---- Sonoff RF Bridge: serial frame from the bridge -> MQTT JSON
  mayhem_reset();
  feed_serial(bytes({0xAA, 0xA4, 0x25, 0x1C, 0x01, 0x40, 0x03, 0x84, 0x01, 0xE2, 0x40, 0x55}));
  SRFBtoX();
  expect_str("SRFB RFIN frame -> JSON", mayhem_published(),
             "{\"raw\":\"251C0140038401E240\",\"delay\":9500,\"val_Tlow\":320,\"val_Thigh\":900,"
             "\"value\":123456,\"origin\":\"/SRFBtoMQTT\"}\n");
  expect_str("SRFB RFIN frame -> ACK to bridge", mayhem_serial_out(), bytes({'\n', 0xAA, 0xA0, 0x55, '\n'}));

  mayhem_reset();
  feed_serial(bytes({0xFF, 0x01, 0xAA, 0xB1, 0x01, 0x02, 0x55}));
  SRFBtoX();
  expect_str("SRFB non-RFIN frame publishes nothing", mayhem_published(), "");

  // ---- LoRa: packet received over the air -> MQTT JSON
  mayhem_reset();
  feed_lora("{\"id\":\"AA:BB\",\"temp\":21.5}");
  LORAtoX();
  expect_str("LORA json packet -> MQTT", mayhem_published(),
             "{\"id\":\"AA:BB\",\"temp\":21.5,\"rssi\":-80,\"snr\":5,\"pferror\":0,\"packetSize\":26,"
             "\"model\":\"LORA_NODE\",\"origin\":\"/LORAtoMQTT/AABB\"}\n");

  mayhem_reset();
  feed_lora("hello world");
  LORAtoX();
  expect_str("LORA text packet -> message", mayhem_published(),
             "{\"message\":\"hello world\",\"rssi\":-80,\"snr\":5,\"pferror\":0,\"packetSize\":11,"
             "\"origin\":\"/LORAtoMQTT\"}\n");

  mayhem_reset();
  feed_lora(bytes({0x01, 0x02, 0xFF}));
  LORAtoX();
  expect_str("LORA binary packet -> hex", mayhem_published(),
             "{\"hex\":\"0102FF\",\"rssi\":-80,\"snr\":5,\"pferror\":0,\"packetSize\":3,"
             "\"origin\":\"/LORAtoMQTT\"}\n");

  mayhem_reset();
  LORAConfig.onlyKnown = true;
  feed_lora(bytes({0x01, 0x02, 0xFF}));
  LORAtoX();
  expect_str("LORA onlyKnown drops unidentified binary", mayhem_published(), "");

  // ---- LoRa: MQTT commands
  mayhem_reset();
  XtoLORA("/commands/MQTTtoLORA", "ping");
  expect_str("LORA plain MQTT -> transmitted", mayhem_lora_out(), "ping");

  mayhem_reset();
  {
    StaticJsonDocument<JSON_MSG_BUFFER_MAX> doc;
    deserializeJson(doc, "{\"hex\":\"CAFE01\"}");
    JsonObject o = doc.as<JsonObject>();
    XtoLORA("/commands/MQTTtoLORA", o);
  }
  expect_str("LORA json hex -> transmitted bytes", mayhem_lora_out(), bytes({0xCA, 0xFE, 0x01}));

  mayhem_reset();
  {
    StaticJsonDocument<JSON_MSG_BUFFER_MAX> doc;
    deserializeJson(doc, "{\"frequency\":433000000,\"spreadingfactor\":9,\"syncword\":\"0x34\"}");
    JsonObject o = doc.as<JsonObject>();
    XtoLORA("/commands/MQTTtoLORA/config", o);
  }
  check("LORA config frequency", LORAConfig.frequency == 433000000);
  check("LORA config spreading factor", LORAConfig.spreadingFactor == 9);
  check("LORA config sync word", LORAConfig.syncWord == 0x34);
  check("LORA config state published", mayhem_published().find("\"frequency\":433000000") != std::string::npos &&
                                           mayhem_published().find("\"syncword\":\"0x34\"") != std::string::npos);

  printf("SELFTEST passed=%d failed=%d\n", passed, failed);
  return failed ? 1 : 0;
}
