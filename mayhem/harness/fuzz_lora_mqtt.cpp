// gatewayLORA.cpp XtoLORA(): MQTT commands to transmit a LoRa packet or to change the radio settings, in the
// plain-payload and JSON forms main.cpp's receivingDATA() dispatches. Byte 0 picks the form/topic.

#include "TheengsCommon.h" // first: its ARDUINOJSON_* macros must match the firmware build

#include <string>

#include "stubs.h"
#include "config_LORA.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 1) return 0;
  mayhem_reset();
  const uint8_t mode = data[0] % 4;
  const std::string payload((const char*)data + 1, size - 1);
  const std::string full = std::string(mqtt_topic) + gateway_name;

  if (mode == 0) {
    XtoLORA(subjectMQTTtoLORA, payload.c_str());
    return 0;
  }
  StaticJsonDocument<JSON_MSG_BUFFER_MAX> doc;
  if (deserializeJson(doc, payload.c_str())) return 0;
  JsonObject obj = doc.as<JsonObject>();
  if (obj.isNull()) return 0;
  const std::string topic = mode == 1 ? std::string(subjectMQTTtoLORA)
                          : mode == 2 ? std::string(subjectMQTTtoLORAset)
                                      : full + subjectMQTTtoLORA;
  XtoLORA(topic.c_str(), obj);
  return 0;
}
