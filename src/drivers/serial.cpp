#include "serial.h"
#include "binio.h"
#include <cstdint>

namespace serial {

void init() {
  const uint16_t COM1 = 0x3F8;

  // Disable interrupts
  outb(COM1 + 1, 0x00);

  // Enable DLAB (set baud rate divisor)
  outb(COM1 + 3, 0x80);

  // Set baud rate divisor to 3 (low byte)
  // 115200 / 3 = 38400 baud
  outb(COM1 + 0, 0x03);

  // High byte of divisor
  outb(COM1 + 1, 0x00);

  // 8 bits, no parity, one stop bit
  outb(COM1 + 3, 0x03);

  // Enable FIFO, clear them, with 14-byte threshold
  outb(COM1 + 2, 0xC7);

  // IRQs enabled, RTS/DSR set
  outb(COM1 + 4, 0x0B);

  // Set loopback mode (for testing)
  outb(COM1 + 4, 0x1E);

  // Test byte
  outb(COM1 + 0, 0xAE);

  // Check if serial is working
  if (inb(COM1 + 0) != 0xAE) {
    // Serial is broken (optional handling)
    return;
  }

  // Exit loopback mode, enable normal operation
  outb(COM1 + 4, 0x0F);
}

void write_char(char c) {
  const uint16_t COM1 = 0x3F8;

  // Wait for transmit buffer to be empty
  while ((inb(COM1 + 5) & 0x20) == 0) {
    // spin
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

} // namespace serial
