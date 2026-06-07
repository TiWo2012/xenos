module;

#include <cstdint>

export module irq.pit;

import binio;
import serial;

export namespace irq {
namespace pit {

void pit_init(uint32_t freq);
void timer_handler();

} // namespace pit
} // namespace irq

namespace irq {
namespace pit {

static volatile uint64_t tick_count = 0;

void pit_init(uint32_t freq) {
  uint32_t divisor = 1193182 / freq;

  outb(0x43, 0x36);

  outb(0x40, divisor & 0xFF);
  outb(0x40, (divisor >> 8) & 0xFF);
}

void timer_handler() {
  tick_count = tick_count + 1;
}

} // namespace pit
} // namespace irq
