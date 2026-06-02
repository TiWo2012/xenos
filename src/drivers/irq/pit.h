#pragma once

#include <cstdint>

namespace irq {
namespace pit {

#define PIT_COMMAND 0X43
#define PIT_CHANNEL0 0X40

void pit_init(uint32_t freq);

} // namespace pit
} // namespace irq
