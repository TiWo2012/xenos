#include <cstdlib>
import binio;
import idt;
import irq.kbd;
import irq.pit;
import heap;
import pmm;
import serial;
import terminal;
import vga;
import vmm;
import utils;
#include <cstddef>
#include <stdint.h>

extern "C" void test_iretq_asm(void);

extern "C" void kernel_main(uint32_t, uint32_t mb_info) {
  serial::init();
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

  serial::printf("initializing vmm\n");
  vmm::init();

  serial::printf("initializing terminal\n");
  terminal::init();

  serial::printf("init heap\n");
  size_t heap_size = pmm::free_frames() * 4096;
  heap::init(heap_size);

  serial::printf("fully booted the os\n");

  serial::printf("dropping identity map\n");

  struct {
    uint16_t limit;
    uint64_t base;
  } __attribute__((packed)) gdtr;
  __asm__ __volatile__("sgdt %0" : "=m"(gdtr));
  gdtr.base = (uint64_t)pmm::phys_to_virt(gdtr.base);
  __asm__ __volatile__("lgdt %0" : : "m"(gdtr));

  uint64_t cr3;
  __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
  uint64_t *pml4 = (uint64_t *)pmm::phys_to_virt(cr3);
  pml4[0] = 0;
  __asm__ __volatile__("invlpg (%0)" : : "r"(0ULL) : "memory");
  serial::printf(
      "identity map dropped, running purely on physmap + higher half\n");

  while (true) {
    __asm__ __volatile__("hlt");
  }
}
