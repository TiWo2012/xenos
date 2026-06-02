#pragma once

#include <cstdint>

namespace irq {
namespace kbd {

#define KBD_DATA    0x60
#define KBD_STATUS  0x64
#define KBD_COMMAND 0x64

void kbd_init();
void keyboard_handler();

} // namespace kbd
} // namespace irq
