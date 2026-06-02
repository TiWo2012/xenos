#pragma once

#include <cstdint>
namespace idt {

void init();
void pic_remap();
void pic_eoi(uint8_t irq);
void pic_unmask_irq(uint8_t irq);
void pic_mask_irq(uint8_t irq);
void irq_register_handler(uint8_t irq, void (*handler)());
void irq_dispatch(uint64_t int_no);

} // namespace idt
