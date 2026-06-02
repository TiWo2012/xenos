#include "drivers/binio.h"
#include "drivers/idt.h"
#include "drivers/irq/pit.h"
#include "drivers/serial.h"
#include "drivers/vga.h"
#include <stdint.h>

static volatile uint64_t tick_count = 0;

static void timer_handler() {
  tick_count = tick_count + 1;
  if (tick_count % 100 == 0) {
    serial::write_string("tick ");
    uint64_t tmp = tick_count;
    char buf[19] = "0x0000000000000000";
    for (int i = 17; i >= 2; i--) {
      int d = tmp & 0xF;
      buf[i] = d < 10 ? '0' + d : 'a' + d - 10;
      tmp >>= 4;
    }
    serial::write_string(buf);
    serial::write_string("\n");
  }
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

  serial::write_string("loading idt\n");
  idt::init();

  serial::write_string("init pit\n");
  irq::pit::pit_init(1000);
  idt::irq_register_handler(0, timer_handler);

  // mask everything, then unmask IRQ0
  outb(0x21, 0xFE);
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

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
