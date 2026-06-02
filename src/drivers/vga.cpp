#include "vga.h"
#include <cstddef>

namespace vga {

volatile char *vga = (volatile char *)0xB8000;

struct vga_index {
  int x, y;
};
vga_index vga_idx;

// y * 80 * x

void write_char(char c) {
  if (c == '\n') {
    vga_idx.x = 0;
    vga_idx.y++;
    return;
  }

  vga[(vga_idx.y * 80 + vga_idx.x) * 2] = c;
  vga[(vga_idx.y * 80 + vga_idx.x) * 2 + 1] = 0x0F;

  vga_idx.x++;

  if (vga_idx.x >= 80) {
    vga_idx.x = 0;
    vga_idx.y++;
  }

  if (vga_idx.y >= 25) {
    vga_idx.y = 0; // temporary
  }
}

void write_string(const char *s) {
  for (int i = 0; s[i] != '\0'; i++) {
    write_char(s[i]);
  }
}

void clear_scr() {
  for (size_t i = 0; i < 80 * 25; i++) {
    write_char(' ');
  }

  vga_idx.x = vga_idx.y = 0;
}

} // namespace vga
