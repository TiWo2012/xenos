import binio;
import idt;
#include "drivers/irq/kbd.h"
import irq.pit;
import heap;
import pmm;
import serial;
#include "drivers/terminal/terminal.h"
import vga;
#include <cstddef>
#include <stdint.h>

extern "C" void test_iretq_asm(void);

extern "C" void kernel_main(uint32_t, uint32_t mb_info) {
  vga::init(mb_info);
  vga::printf("clearing vga screen\n");
  serial::printf("clearing vga screen\n");
  vga::clear_scr();

  serial::printf("hello world from serial\n");

  serial::printf("remaping pic\n");
  idt::pic_remap();

  uint64_t rsp_val;
  __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp_val));
  serial::printf("rsp before idt_init=0x%lx\n", rsp_val);

  serial::printf("loading idt\n");
  idt::init();

  serial::printf("init pit\n");
  irq::pit::pit_init(1000);
  idt::irq_register_handler(0, irq::pit::timer_handler);

  serial::printf("init kbd\n");
  irq::kbd::kbd_init();
  idt::irq_register_handler(1, irq::kbd::keyboard_handler);

  // mask everything, then unmask IRQ0 and IRQ1
  outb(0x21, 0xFC);
  outb(0xA1, 0xFF);

  __asm__ __volatile__("sti");

  __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp_val));
  serial::printf("rsp after sti=0x%lx\n", rsp_val);

  serial::printf("test_iretq_asm\n");
  __asm__ __volatile__("mov %%rsp, %0" : "=r"(rsp_val));
  serial::printf("rsp before call=0x%lx\n", rsp_val);
  test_iretq_asm();
  serial::printf("survived iretq_asm\n");

  serial::printf("init pmm\n");
  pmm::init(mb_info);

  vga::color white;
  white.r = white.g = white.b = white.a = 0xFF;
  vga::color black;
  black.raw = 0;

  const char* msg = "hello world";
  uint32_t msg_x = 10;
  uint32_t msg_y = 10;
  for (int i = 0; msg[i] != '\0'; i++) {
    vga::put_char(msg_x, msg_y, msg[i], white, black);
    msg_x += 8;
  }
  serial::printf("hello world drawn\n");

  serial::printf("initializing terminal\n");
  terminal::init();

  serial::printf("init heap\n");
  size_t heap_size = pmm::free_frames() * 4096;
  heap::init(heap_size);

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
