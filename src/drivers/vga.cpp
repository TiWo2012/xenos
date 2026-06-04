#include "vga.h"
#include "../utils/string.h"
#include "binio.h"
#include <cstddef>
#include <stdarg.h>

namespace vga {

volatile char *vga = (volatile char *)0xB8000;

struct vga_index {
  int x, y;
};
vga_index vga_idx;

static void update_cursor() {
  uint16_t pos = vga_idx.y * 80 + vga_idx.x;
  outb(0x3D4, 0x0F);
  outb(0x3D5, pos & 0xFF);
  outb(0x3D4, 0x0E);
  outb(0x3D5, (pos >> 8) & 0xFF);
}

static void set_cursor_shape(uint8_t start, uint8_t end) {
  outb(0x3D4, 0x0A);
  outb(0x3D5, start);
  outb(0x3D4, 0x0B);
  outb(0x3D5, end);
}

// y * 80 * x

void write_char(char c) {
  if (c == '\n') {
    vga_idx.x = 0;
    vga_idx.y++;
    update_cursor();
    return;
  }

  if (c == '\b') {
    backspace();
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

  update_cursor();
}

void backspace() {
  if (vga_idx.x == 0 && vga_idx.y == 0) {
    return;
  }

  if (vga_idx.x == 0) {
    vga_idx.y--;
    vga_idx.x = 79;
  } else {
    vga_idx.x--;
  }

  vga[(vga_idx.y * 80 + vga_idx.x) * 2] = ' ';
  vga[(vga_idx.y * 80 + vga_idx.x) * 2 + 1] = 0x0F;

  update_cursor();
}

void write_string(const char *s) {
  for (int i = 0; s[i] != '\0'; i++) {
    write_char(s[i]);
  }
}

void printf(const char *format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  utils::string::vsprintf(buf, format, args);
  va_end(args);
  write_string(buf);
}

void clear_scr() {
  for (size_t i = 0; i < 80 * 25; i++) {
    vga[i * 2] = ' ';
    vga[i * 2 + 1] = 0x0F;
  }

  vga_idx.x = vga_idx.y = 0;

  set_cursor_shape(0, 15);
  update_cursor();
}

void set_cursor_visible() {
  set_cursor_shape(0, 15);
  update_cursor();
}

} // namespace vga
