// isr.cpp
#include "serial.h"
#include <stdint.h>

// match EXACT push order (reverse of pops!)
struct Registers {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t int_no, err_code;
};

static void print_hex(uint64_t val) {
  char buf[19] = "0x0000000000000000";
  for (int i = 17; i >= 2; i--) {
    int digit = val & 0xF;
    buf[i] = digit < 10 ? '0' + digit : 'a' + digit - 10;
    val >>= 4;
  }
  serial::write_string(buf);
}

extern "C" void isr_handler(Registers *regs) {
  uint64_t *frame = (uint64_t *)regs;

  struct { uint16_t limit; uint64_t base; } __attribute__((packed)) gdtr;
  __asm__ __volatile__("sgdt %0" : "=m"(gdtr));

  serial::write_string("rsp=");
  print_hex((uint64_t)regs);
  serial::write_string(" [15]=");
  print_hex(frame[15]);
  serial::write_string(" [16]=");
  print_hex(frame[16]);
  serial::write_string(" [17]=");
  print_hex(frame[17]);
  serial::write_string(" [18]=");
  print_hex(frame[18]);
  serial::write_string(" [19]=");
  print_hex(frame[19]);
  serial::write_string("\n");

  uint64_t rip  = frame[17];
  uint64_t cs   = frame[18];
  uint64_t rfl  = frame[19];

  serial::write_string("I:");
  print_hex(regs->int_no);
  serial::write_string(" ec:");
  print_hex(regs->err_code);
  serial::write_string(" rip:");
  print_hex(rip);
  serial::write_string(" cs:");
  print_hex(cs);
  serial::write_string(" rfl:");
  print_hex(rfl);
  serial::write_string(" gdt=");
  print_hex(gdtr.base);
  serial::write_string("\n");
}
