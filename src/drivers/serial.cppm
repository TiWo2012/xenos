module;

#include <cstdint>
#include <stdarg.h>

export module serial;

import binio;
import utils.string;

export namespace serial {

void init() {
  const uint16_t COM1 = 0x3F8;

  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x80);
  outb(COM1 + 0, 0x03);
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x03);
  outb(COM1 + 2, 0xC7);
  outb(COM1 + 4, 0x0B);
  outb(COM1 + 4, 0x1E);
  outb(COM1 + 0, 0xAE);

  if (inb(COM1 + 0) != 0xAE) {
    return;
  }

  outb(COM1 + 4, 0x0F);
}

void write_char(char c) {
  const uint16_t COM1 = 0x3F8;

  while ((inb(COM1 + 5) & 0x20) == 0) {
  }

  outb(COM1, (uint8_t)c);
}

void write_string(const char *s) {
  while (*s != '\0') {
    write_char(*s);
    s++;
  }
}

void print_hex(uint64_t val) {
  char buf[19] = "0x0000000000000000";
  for (int i = 17; i >= 2; i--) {
    int digit = val & 0xF;
    buf[i] = digit < 10 ? '0' + digit : 'a' + digit - 10;
    val >>= 4;
  }
  serial::write_string(buf);
}

void write_dec(uint64_t val) {
  char buf[21];
  int i = 20;
  buf[i] = '\0';
  if (val == 0) {
    write_char('0');
    return;
  }
  while (val > 0 && i > 0) {
    i--;
    buf[i] = '0' + (val % 10);
    val /= 10;
  }
  write_string(&buf[i]);
}

void printf(const char *format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  utils::string::vsprintf(buf, format, args);
  va_end(args);
  write_string(buf);
}

}
