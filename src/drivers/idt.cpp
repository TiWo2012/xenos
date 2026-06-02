// idt.cpp
#include "idt.h"
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

  load_idt(&idt_ptr);
}
} // namespace idt
