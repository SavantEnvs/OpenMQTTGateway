// gatewaySRFB.cpp XtoSRFB(): MQTT commands that make the RF bridge transmit, both the plain-payload form and
// the JSON form (the dispatch main.cpp's receivingDATA() performs). Byte 0 picks the topic/form.

#include "TheengsCommon.h" // first: its ARDUINOJSON_* macros must match the firmware build

#include <string>

#include "stubs.h"

extern void XtoSRFB(const char* topicOri, const char* datacallback);
extern void XtoSRFB(const char* topicOri, JsonObject& RFdata);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 1) return 0;
  mayhem_reset();
  const uint8_t mode = data[0] % 4;
  const std::string payload((const char*)data + 1, size - 1);
  const std::string full = std::string(mqtt_topic) + gateway_name;

  switch (mode) {
    case 0:
      XtoSRFB("/commands/MQTTtoSRFB", payload.c_str());
      break;
    case 1:
      XtoSRFB("/commands/MQTTtoSRFB/Raw", payload.c_str());
      break;
    default: {
      StaticJsonDocument<JSON_MSG_BUFFER_MAX> doc;
      if (deserializeJson(doc, payload.c_str())) return 0;
      JsonObject obj = doc.as<JsonObject>();
      if (obj.isNull()) return 0;
      const std::string topic = mode == 2 ? "/commands/MQTTtoSRFB" : full + "/commands/MQTTtoSRFB";
      XtoSRFB(topic.c_str(), obj);
    }
  }
  return 0;
}
