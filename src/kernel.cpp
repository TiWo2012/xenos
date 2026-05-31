#include <stdint.h>

static inline void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void serial_init() {
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

void serial_write_char(char c) {
  const uint16_t COM1 = 0x3F8;

  // Wait for transmit buffer to be empty
  while ((inb(COM1 + 5) & 0x20) == 0) {
    // spin
  }

  outb(COM1, (uint8_t)c);
}

extern "C" void kernel_main(uint32_t magic, uint32_t mb_info) {
  volatile char *vga = (volatile char *)0xB8000;

  const char *msg = "C++ kernel is alive";

  for (uint32_t i = 0; msg[i] != '\0'; i++) {
    vga[i * 2] = msg[i];
    vga[i * 2 + 1] = 0x0F;
  }

  serial_write_char('h');

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
