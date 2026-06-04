#include "drivers/binio.h"
#include "drivers/idt.h"
#include "drivers/irq/kbd.h"
#include "drivers/irq/pit.h"
#include "drivers/serial.h"
#include "drivers/terminal/terminal.h"
#include "drivers/vga.h"
#include <stdint.h>

static void put_hex(uint64_t val) {
  for (int i = 60; i >= 0; i -= 4) {
    int d = (val >> i) & 0xF;
    serial::write_char(d < 10 ? '0' + d : 'a' + d - 10);
  }
}

extern "C" void test_iretq_asm(void);

extern "C" void kernel_main(uint32_t magic, uint32_t mb_info) {
  vga::write_string("clearing vga screen\n");
  serial::write_string("clearing vga screen\n");
  vga::clear_scr();

  serial::write_string("hello world from serial\n");

  serial::write_string("remaping pic\n");
  idt::pic_remap();

  uint64_t rsp_val;
  __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp_val));
  serial::write_string("rsp before idt_init=");
  put_hex(rsp_val);
  serial::write_string("\n");

  serial::write_string("loading idt\n");
  idt::init();

  serial::write_string("init pit\n");
  irq::pit::pit_init(1000);
  idt::irq_register_handler(0, irq::pit::timer_handler);

  serial::write_string("init kbd\n");
  irq::kbd::kbd_init();
  idt::irq_register_handler(1, irq::kbd::keyboard_handler);

  // mask everything, then unmask IRQ0 and IRQ1
  outb(0x21, 0xFC);
  outb(0xA1, 0xFF);

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

  vga::write_string("welcome to xenos\n");

  serial::write_string("initializing terminal\n");
  terminal::init();

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
