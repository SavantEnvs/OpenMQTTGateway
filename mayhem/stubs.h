#pragma once
// Host-side environment for OpenMQTTGateway's gateway modules: scripted radio/UART input and a sink for
// everything the firmware would publish or transmit.
#include <stddef.h>
#include <stdint.h>
#include <string>

void mayhem_reset();
void mayhem_serial_feed(const uint8_t* data, size_t n);
void mayhem_lora_feed(const uint8_t* data, size_t n);
const std::string& mayhem_published();
const std::string& mayhem_serial_out();
const std::string& mayhem_lora_out();
void mayhem_set_discovery(bool on);
