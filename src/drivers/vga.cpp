#include "vga.h"
#include <cstdint>

volatile char *vga = (volatile char *)0xB8000;
uint16_t vga_index = 0;

void vga_write_char(char c) {
  vga[vga_index * 2] = c;
  vga[vga_index * 2 + 1] = 0x0F;
  vga_index++;
}

void vga_write_string(const char *s) {
  for (int i = 0; s[i] != '\0'; i++) {
    vga[vga_index * 2] = s[i];
    vga[vga_index * 2 + 1] = 0x0F;
    vga_index++;
  }
}
