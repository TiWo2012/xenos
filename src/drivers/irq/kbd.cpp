#include "kbd.h"
#include "../binio.h"
#include "../serial.h"
#include <cstdint>

namespace irq {
namespace kbd {
#define DEBUG_KBD_SCANCODE false

void kbd_init() {
  serial::write_string("kbd_init (stub)\n");

  uint8_t scancode;

  scancode = inb(0x60);

  serial::write_string("scancode: ");
  serial::print_hex(scancode);
}

void keyboard_handler() {
#if DEBUG_KBD_SCANCODE
  serial::write_string("kbd fired: ");

  uint8_t scancode;

  scancode = inb(0x60);

  serial::write_string("scancode(");
  serial::print_hex(scancode);
  serial::write_string(")\n");
#endif
}
} // namespace kbd
} // namespace irq
