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

extern "C" void isr0();

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

  // only one interrupt for now
  set_idt_entry(0, isr0);

  load_idt(&idt_ptr);
}
} // namespace idt
