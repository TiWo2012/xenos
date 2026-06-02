#include "drivers/binio.h"
#include "drivers/idt.h"
#include "drivers/serial.h"
#include "drivers/vga.h"
#include <stdint.h>

static void pic_mask_all() {
  outb(0x21, 0xFF);
  outb(0xA1, 0xFF);
}

static void put_hex(uint64_t val) {
  for (int i = 60; i >= 0; i -= 4) {
    int d = (val >> i) & 0xF;
    serial::write_char(d < 10 ? '0' + d : 'a' + d - 10);
  }
}

extern "C" void test_iretq_asm(void);

extern "C" void kernel_main(uint32_t magic, uint32_t mb_info) {

  vga::write_string("hello\nworld");
  serial::write_string("hello world from serial\n");

  serial::write_string("remaping pic\n");
  idt::pic_remap();

  uint64_t rsp_val;
  __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp_val));
  serial::write_string("rsp before idt_init=");
  put_hex(rsp_val);
  serial::write_string("\n");

  serial::write_string("enabled  interrupts\n");
  idt::init();
  pic_mask_all();

  __asm__ __volatile__("sti");

  __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp_val));
  serial::write_string("rsp after sti=");
  put_hex(rsp_val);
  serial::write_string("\n");

  serial::write_string("test_iretq_asm\n");
  __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp_val));
  serial::write_string("rsp before call=");
  put_hex(rsp_val);
  serial::write_string("\n");
  test_iretq_asm();
  serial::write_string("survived iretq_asm\n");

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
