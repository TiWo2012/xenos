#include "drivers/binio.h"
#include "drivers/idt.h"
#include "drivers/irq/kbd.h"
#include "drivers/irq/pit.h"
#include "drivers/mem/heap.h"
#include "drivers/mem/pmm.h"
#include "drivers/serial.h"
#include "drivers/terminal/terminal.h"
#include "drivers/vga.h"
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

  vga::printf("welcome to xenos\n");

  vga::color colors[3];
  colors[0].r = 0xFF; colors[0].g = 0x00; colors[0].b = 0x00; colors[0].a = 0xFF;
  colors[1].r = 0x00; colors[1].g = 0xFF; colors[1].b = 0x00; colors[1].a = 0xFF;
  colors[2].r = 0x00; colors[2].g = 0x00; colors[2].b = 0xFF; colors[2].a = 0xFF;

  for (uint32_t y = 0; y < vga::fb_info.height; y++) {
    for (uint32_t x = 0; x < vga::fb_info.width; x++) {
      uint32_t stripe = (x * 3) / vga::fb_info.width;
      vga::put_pixel(x, y, colors[stripe]);
    }
  }
  serial::printf("stripes drawn\n");

  serial::printf("initializing terminal\n");
  terminal::init();

  serial::printf("init heap\n");
  size_t heap_size = pmm::free_frames() * 4096;
  heap::init(heap_size);

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
