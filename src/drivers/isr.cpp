// isr.cpp
#include "idt.h"
#include "serial.h"
#include <stdint.h>

// match EXACT push order (reverse of pops!)
struct Registers {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t int_no, err_code;
};

extern "C" void isr_handler(Registers *regs) {
  if (regs->int_no >= 32) {
    idt::irq_dispatch(regs->int_no);
    return;
  }

  uint64_t *frame = (uint64_t *)regs;

  struct {
    uint16_t limit;
    uint64_t base;
  } __attribute__((packed)) gdtr;
  __asm__ __volatile__("sgdt %0" : "=m"(gdtr));

  serial::write_string("rsp=");
  serial::print_hex((uint64_t)regs);
  serial::write_string(" [15]=");
  serial::print_hex(frame[15]);
  serial::write_string(" [16]=");
  serial::print_hex(frame[16]);
  serial::write_string(" [17]=");
  serial::print_hex(frame[17]);
  serial::write_string(" [18]=");
  serial::print_hex(frame[18]);
  serial::write_string(" [19]=");
  serial::print_hex(frame[19]);
  serial::write_string("\n");

  uint64_t rip = frame[17];
  uint64_t cs = frame[18];
  uint64_t rfl = frame[19];

  serial::write_string("I:");
  serial::print_hex(regs->int_no);
  serial::write_string(" ec:");
  serial::print_hex(regs->err_code);
  serial::write_string(" rip:");
  serial::print_hex(rip);
  serial::write_string(" cs:");
  serial::print_hex(cs);
  serial::write_string(" rfl:");
  serial::print_hex(rfl);
  serial::write_string(" gdt=");
  serial::print_hex(gdtr.base);
  serial::write_string("\n");
}
