#include "drivers/idt.h"
#include "drivers/serial.h"
#include "drivers/vga.h"
#include <stdint.h>

extern "C" void kernel_main(uint32_t magic, uint32_t mb_info) {

  vga::write_string("hello\n world");
  serial::write_string("hello world from serial\n");

  serial::write_string("enabled  interrupts\n");
  idt::init();

  __asm__ __volatile__("sti");

  __asm__ __volatile__("int $0");

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
