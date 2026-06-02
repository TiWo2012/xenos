#include "pit.h"
#include "../binio.h"
#include "../serial.h"
#include <cstdint>

namespace irq {
namespace pit {

#define DEBUG_PIT_TICK false

static volatile uint64_t tick_count = 0;

void pit_init(uint32_t freq) {
  uint32_t divisor = 1193182 / freq;

  outb(PIT_COMMAND, 0x36); // channel 0, lobyte/hibyte, mode 3 (square wave)

  outb(PIT_CHANNEL0, divisor & 0xFF);        // low byte
  outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF); // high byte
}

void timer_handler() {
  tick_count = tick_count + 1;

#if DEBUG_PIT_TICK
  if (tick_count % 100 == 0) {
    serial::write_string("tick ");
    uint64_t tmp = tick_count;
    char buf[19] = "0x0000000000000000";
    for (int i = 17; i >= 2; i--) {
      int d = tmp & 0xF;
      buf[i] = d < 10 ? '0' + d : 'a' + d - 10;
      tmp >>= 4;
    }
    serial::write_string(buf);
    serial::write_string("\n");
  }
#endif
}
} // namespace pit
} // namespace irq
