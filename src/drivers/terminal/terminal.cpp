#include "terminal.h"
#include "../../utils/string.h"
#include "../serial.h"
#include "../vga.h"
#include <cstdint>

namespace terminal {

static char buf[256];
static uint8_t buf_idx = 0;

void init() {
  buf_idx = 0;
  for (int i = 0; i < 256; i++) {
    buf[i] = 0;
  }
}

// PS/2 scancode set 1 → US ANSI layout (unshifted)
const char keymap[256] = {
    0,    0,   '1', '2',  '3',  '4', '5', '6', // 0x00-0x07
    '7',  '8', '9', '0',  '-',  '=', 0,   0,   // 0x08-0x0F
    'q',  'w', 'e', 'r',  't',  'y', 'u', 'i', // 0x10-0x17
    'o',  'p', '[', ']',  '\n', 0,   'a', 's', // 0x18-0x1F
    'd',  'f', 'g', 'h',  'j',  'k', 'l', ';', // 0x20-0x27
    '\'', '`', 0,   '\\', 'z',  'x', 'c', 'v', // 0x28-0x2F
    'b',  'n', 'm', ',',  '.',  '/', 0,   '*', // 0x30-0x37
    0,    ' ', 0,   0,    0,    0,   0,   0,   // 0x38-0x3F
    0,    0,   0,   0,    0,    0,   0,   '7', // 0x40-0x47
    '8',  '9', '-', '4',  '5',  '6', '+', '1', // 0x48-0x4F
    '2',  '3', '0', '.',                       // 0x50-0x53
};

void send_key(uint8_t scanCode) {
  auto key = keymap[scanCode];

  // break codes (key release) = make code + 0x80 → ignore
  if (scanCode & 0x80)
    return;

  if (key == 0)
    return;

  vga::write_char(key);

  serial::write_string("pressed key: ");
  serial::write_char(key);
  serial::write_char('\n');

  if (buf_idx < 255) {
    buf[buf_idx++] = key;
  }
  buf[buf_idx] = '\0';

  serial::write_string("buf_idx: ");
  serial::write_dec(buf_idx);
  serial::write_char('\n');
  serial::write_string("buf: ");
  serial::write_string(buf);
  serial::write_char('\n');

  if (key == '\n') {
    buf[buf_idx - 1] = '\0';
    process_command();
    buf_idx = 0;
    for (int i = 0; i < 256; i++) {
      buf[i] = 0;
    }
  }
}

extern "C" void asm_shutdown();
void process_command() {
  if (utils::string::strcmp(buf, "exit") == 0) {
    asm_shutdown();
  }
}

} // namespace terminal
