#include "pit.h"
#include "../binio.h"
#include <cstdint>

namespace irq {
namespace pit {
void pit_init(uint32_t freq) {
  uint32_t divisor = 1193182 / freq;

  outb(PIT_COMMAND, 0x36); // channel 0, lobyte/hibyte, mode 3 (square wave)

  outb(PIT_CHANNEL0, divisor & 0xFF);        // low byte
  outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF); // high byte
}
} // namespace pit
} // namespace irq
