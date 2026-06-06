module;

#include <cstdint>

export module irq.kbd;

import binio;
import serial;
import terminal;

export namespace irq {
namespace kbd {

void kbd_init();
void keyboard_handler();

} // namespace kbd
} // namespace irq

namespace irq {
namespace kbd {

void kbd_init() {
  serial::write_string("kbd_init (stub)\n");

  uint8_t scancode;

  scancode = inb(0x60);

  serial::write_string("scancode: ");
  serial::print_hex(scancode);
}

void keyboard_handler() {
  uint8_t scancode;

  scancode = inb(0x60);

  terminal::send_key(scancode);
}

} // namespace kbd
} // namespace irq
