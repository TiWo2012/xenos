#include "terminal.h"
#include "../../utils/string.h"
#include "../serial.h"
#include "../vga.h"
#include "memory.h"
#include <cstdint>

namespace terminal {

#define DEBUG_TERM_ECHO_KEY false

static char buf[256];
static uint8_t buf_idx = 0;

static void write_prompt() {
  vga::write_char('>');
  vga::write_char(' ');
}

void init() {
  buf_idx = 0;
  utils::memory::memset(buf, 0, sizeof(buf));
  write_prompt();
}

// PS/2 scancode set 1 → US ANSI layout (unshifted)
const char keymap[256] = {
    0,    0,   '1', '2',  '3',  '4', '5', '6', // 0x00-0x07
    '7',  '8', '9', '0',  '-',  '=', 8,   0,   // 0x08-0x0F
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

  if (key == 8) {
    if (buf_idx > 0) {
      buf_idx--;
      buf[buf_idx] = '\0';
      vga::backspace();
    }
#if DEBUG_TERM_ECHO_KEY
  serial::printf("buf_idx: %u\n", buf_idx);
  serial::printf("buf: %s\n", buf);
#endif
}

  vga::write_char(key);

#if DEBUG_TERM_ECHO_KEY
    serial::printf("pressed key: %c\n", key);
#endif

  if (key == '\n') {
    if (buf_idx > 0) {
      buf[buf_idx] = '\0';
    } else {
      buf[0] = '\0';
    }

#if DEBUG_TERM_ECHO_KEY
    serial::printf("buf_idx: %u\n", buf_idx);
    serial::printf("buf: %s\n", buf);
#endif

    process_command();
    buf_idx = 0;
    utils::memory::memset(buf, 0, sizeof(buf));
    write_prompt();
    return;
  }

  if (buf_idx < sizeof(buf) - 1) {
    buf[buf_idx++] = key;
    buf[buf_idx] = '\0';
  }

#if DEBUG_TERM_ECHO_KEY
    serial::printf("buf_idx: %u\n", buf_idx);
    serial::printf("buf: %s\n", buf);
#endif
}

extern "C" void asm_shutdown();

void process_command() {
  if (utils::string::strcmp(buf, "exit") == 0) {
    asm_shutdown();
  } else if (utils::string::strcmp(buf, "clear") == 0) {
    vga::clear_scr();
  }
}

} // namespace terminal
