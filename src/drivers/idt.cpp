// idt.cpp
#include "idt.h"
import binio;
#include <stdint.h>

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

extern "C" {
void isr0();
void isr1();
void isr2();
void isr3();
void isr4();
void isr5();
void isr6();
void isr7();
void isr8();
void isr9();
void isr10();
void isr11();
void isr12();
void isr13();
void isr14();
void isr15();
void isr16();
void isr17();
void isr18();
void isr19();
void isr20();
void isr21();
void isr22();
void isr23();
void isr24();
void isr25();
void isr26();
void isr27();
void isr28();
void isr29();
void isr30();
void isr31();
void isr32();
void isr33();
}

IDTEntry idt[256];
IDTPointer idt_ptr;

static void set_idt_entry(int n, void (*handler)()) {
  uint64_t addr = (uint64_t)handler;

  idt[n].offset_low = addr & 0xFFFF;
  idt[n].selector = 0x08; // kernel code segment
  idt[n].ist = 0;
  idt[n].type_attr = 0x8E; // present, ring 0, interrupt gate
  idt[n].offset_mid = (addr >> 16) & 0xFFFF;
  idt[n].offset_high = (addr >> 32) & 0xFFFFFFFF;
  idt[n].zero = 0;
}

extern "C" void load_idt(IDTPointer *);

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

  // save masks
  asm volatile("inb %1, %0" : "=a"(a1) : "Nd"(0x21));
  asm volatile("inb %1, %0" : "=a"(a2) : "Nd"(0xA1));

  // start initialization
  outb(0x20, 0x11);
  outb(0xA0, 0x11);

  // set vector offsets
  outb(0x21, 0x20); // master -> 32
  outb(0xA1, 0x28); // slave -> 40

  // tell master about slave at IRQ2
  outb(0x21, 0x04);
  outb(0xA1, 0x02);

  // set 8086 mode
  outb(0x21, 0x01);
  outb(0xA1, 0x01);

  // restore masks
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
