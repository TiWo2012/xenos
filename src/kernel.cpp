#include "drivers/serial.h"
#include "drivers/vga.h"
#include <stdint.h>

extern "C" void kernel_main(uint32_t magic, uint32_t mb_info) {

  vga_write_string("hello world from vga\n");

  serial_write_string("hello world from serial\n");

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
