#pragma once

#include <cstdint>

namespace terminal {
void init();
void send_key(uint8_t scanCode);
void process_command();
} // namespace terminal
