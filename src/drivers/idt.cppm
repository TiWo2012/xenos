module;

#include <stdint.h>

export module idt;

import binio;

extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();
extern "C" void isr32();
extern "C" void isr33();

export namespace idt {

void init();
void pic_remap();
void pic_eoi(uint8_t irq);
void pic_unmask_irq(uint8_t irq);
void pic_mask_irq(uint8_t irq);
void irq_register_handler(uint8_t irq, void (*handler)());
void irq_dispatch(uint64_t int_no);

} // namespace idt

namespace idt {

struct IDTEntry {
  uint16_t offset_low;
  uint16_t selector;
  uint8_t ist;
  uint8_t type_attr;
  uint16_t offset_mid;
  uint32_t offset_high;
  uint32_t zero;
} __attribute__((packed));

struct IDTPointer {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

extern "C" void load_idt(IDTPointer *);

IDTEntry idt[256];
IDTPointer idt_ptr;

static void set_idt_entry(int n, void (*handler)()) {
  uint64_t addr = (uint64_t)handler;

  idt[n].offset_low = addr & 0xFFFF;
  idt[n].selector = 0x08;
  idt[n].ist = 0;
  idt[n].type_attr = 0x8E;
  idt[n].offset_mid = (addr >> 16) & 0xFFFF;
  idt[n].offset_high = (addr >> 32) & 0xFFFFFFFF;
  idt[n].zero = 0;
}

void init() {
  idt_ptr.limit = sizeof(idt) - 1;
  idt_ptr.base = (uint64_t)&idt;

  set_idt_entry(0, isr0);
  set_idt_entry(1, isr1);
  set_idt_entry(2, isr2);
  set_idt_entry(3, isr3);
  set_idt_entry(4, isr4);
  set_idt_entry(5, isr5);
  set_idt_entry(6, isr6);
  set_idt_entry(7, isr7);
  set_idt_entry(8, isr8);
  set_idt_entry(9, isr9);
  set_idt_entry(10, isr10);
  set_idt_entry(11, isr11);
  set_idt_entry(12, isr12);
  set_idt_entry(13, isr13);
  set_idt_entry(14, isr14);
  set_idt_entry(15, isr15);
  set_idt_entry(16, isr16);
  set_idt_entry(17, isr17);
  set_idt_entry(18, isr18);
  set_idt_entry(19, isr19);
  set_idt_entry(20, isr20);
  set_idt_entry(21, isr21);
  set_idt_entry(22, isr22);
  set_idt_entry(23, isr23);
  set_idt_entry(24, isr24);
  set_idt_entry(25, isr25);
  set_idt_entry(26, isr26);
  set_idt_entry(27, isr27);
  set_idt_entry(28, isr28);
  set_idt_entry(29, isr29);
  set_idt_entry(30, isr30);
  set_idt_entry(31, isr31);
  set_idt_entry(32, isr32);
  set_idt_entry(33, isr33);

  load_idt(&idt_ptr);
}

void pic_remap() {
  uint8_t a1, a2;

  asm volatile("inb %1, %0" : "=a"(a1) : "Nd"(0x21));
  asm volatile("inb %1, %0" : "=a"(a2) : "Nd"(0xA1));

  outb(0x20, 0x11);
  outb(0xA0, 0x11);

  outb(0x21, 0x20);
  outb(0xA1, 0x28);

  outb(0x21, 0x04);
  outb(0xA1, 0x02);

  outb(0x21, 0x01);
  outb(0xA1, 0x01);

  outb(0x21, a1);
  outb(0xA1, a2);
}

void pic_eoi(uint8_t irq) {
  if (irq >= 8) {
    outb(0xA0, 0x20);
  }

  outb(0x20, 0x20);
}

void (*irq_handlers[16])() = {nullptr};

void irq_register_handler(uint8_t irq, void (*handler)()) {
  if (irq < 16) {
    irq_handlers[irq] = handler;
  }
}

void irq_dispatch(uint64_t int_no) {
  if (int_no >= 32 && int_no < 48) {
    uint8_t irq = int_no - 32;
    pic_eoi(irq);
    if (irq_handlers[irq]) {
      irq_handlers[irq]();
    }
  }
}

void pic_unmask_irq(uint8_t irq) {
  if (irq < 8) {
    outb(0x21, inb(0x21) & ~(1 << irq));
  } else {
    outb(0xA1, inb(0xA1) & ~(1 << (irq - 8)));
  }
}

void pic_mask_irq(uint8_t irq) {
  if (irq < 8) {
    outb(0x21, inb(0x21) | (1 << irq));
  } else {
    outb(0xA1, inb(0xA1) | (1 << (irq - 8)));
  }
}

} // namespace idt
