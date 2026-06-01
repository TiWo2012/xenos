// isr.cpp
#include "serial.h"
#include <stdint.h>

// match EXACT push order (reverse of pops!)
struct Registers {
  uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
  uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
  uint64_t int_no, err_code;
};

extern "C" void isr_handler(Registers *regs) {
  serial::write_string("INTERRUPT: ");

  char num = '0' + (regs->int_no % 10);
  serial::write_char(num);

  serial::write_string("\n");

  while (1) {
    asm volatile("hlt");
  }
}
