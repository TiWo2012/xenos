module;

#include <cstdint>
#include <cstddef>

export module terminal;

import utils.string;
import pmm;
import serial;
import vga;
import utils.memory;

extern "C" void asm_shutdown();

export namespace terminal {

void init();
void send_key(uint8_t scanCode);
void process_command();

} // namespace terminal

namespace terminal {

static char buf[256];
static uint8_t buf_idx = 0;

static void write_prompt() {
  vga::write_char('>');
  vga::write_char(' ');
}

void init() {
  buf_idx = 0;
  utils::memory::memset(buf, 0, sizeof(buf));
  vga::set_cursor_visible();
  write_prompt();
}

static const char keymap[256] = {
    0,    0,   '1', '2',  '3',  '4', '5', '6',
    '7',  '8', '9', '0',  '-',  '=', 8,   0,
    'q',  'w', 'e', 'r',  't',  'y', 'u', 'i',
    'o',  'p', '[', ']',  '\n', 0,   'a', 's',
    'd',  'f', 'g', 'h',  'j',  'k', 'l', ';',
    '\'', '`', 0,   '\\', 'z',  'x', 'c', 'v',
    'b',  'n', 'm', ',',  '.',  '/', 0,   '*',
    0,    ' ', 0,   0,    0,    0,   0,   0,
    0,    0,   0,   0,    0,    0,   0,   '7',
    '8',  '9', '-', '4',  '5',  '6', '+', '1',
    '2',  '3', '0', '.',
};

void send_key(uint8_t scanCode) {
  auto key = keymap[scanCode];

  if (scanCode & 0x80) {
    return;
  }

  if (key == 0) {
    return;
  }

  if (key == 8) {
    if (buf_idx > 0) {
      buf_idx--;
      buf[buf_idx] = '\0';
      vga::backspace();
    }
    return;
  }

  vga::write_char(key);

  if (key == '\n') {
    if (buf_idx > 0) {
      buf[buf_idx] = '\0';
    } else {
      buf[0] = '\0';
    }

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
}

void process_command() {
  if (utils::string::strcmp(buf, "exit") == 0) {
    asm_shutdown();
  } else if (utils::string::strcmp(buf, "clear") == 0) {
    vga::clear_scr();
  } else if (utils::string::strcmp(buf, "mem") == 0) {
    size_t total = pmm::total_frames();
    size_t free = pmm::free_frames();
    size_t used = total - free;
    char line[64];
    utils::string::sprintf(line, "pmm: total=%u  free=%u  used=%u\n", total,
                           free, used);
    vga::write_string(line);
    serial::printf("%s", line);
  } else if (utils::string::strcmp(buf, "") == 0) {
  } else {
    vga::write_string("command does not exist\n");
    serial::printf("invalid cmd: %s\n", buf);
  }
}

} // namespace terminal
