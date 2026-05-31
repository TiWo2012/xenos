#include <stdint.h>

extern "C" void kernel_main(uint32_t magic, uint32_t mb_info);

extern "C" void boot_main(uint32_t magic, uint32_t mb_info) {
  kernel_main(magic, mb_info);

  // if kernel returns → halt forever
  while (true) {
    __asm__ __volatile__("hlt");
  }
}
