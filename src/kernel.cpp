#include "drivers/serial.h"
#include <stdint.h>

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
