#include "vga.h"
#include "font.h"
#include "../utils/string.h"
#include "../utils/memory.h"
#include "binio.h"
#include "serial.h"
#include <cstddef>
#include <stdarg.h>

namespace vga {

framebuffer_info fb_info;
volatile uint32_t *fb = nullptr;

volatile char *vga = (volatile char *)0xB8000;

struct vga_index {
  int x, y;
};
vga_index vga_idx;

struct fb_cursor {
  uint32_t x, y;
};
fb_cursor fb_cur;

color fb_fg;
color fb_bg;

void init(uint32_t mb_info_addr) {
  uint8_t *mb = (uint8_t *)(uint64_t)mb_info_addr;
  uint32_t total_size = *(uint32_t *)mb;

  uint32_t offset = 8;
  while (offset + 8 <= total_size) {
    uint32_t tag_type = *(uint32_t *)(mb + offset);
    uint32_t tag_size = *(uint32_t *)(mb + offset + 4);

    if (tag_type == 0) {
      break;
    }

    if (tag_type == 8 && tag_size >= 32) {
      fb_info.addr = *(uint64_t *)(mb + offset + 8);
      fb_info.pitch = *(uint32_t *)(mb + offset + 16);
      fb_info.width = *(uint32_t *)(mb + offset + 20);
      fb_info.height = *(uint32_t *)(mb + offset + 24);
      fb_info.bpp = *(uint8_t *)(mb + offset + 28);
      fb = (volatile uint32_t *)fb_info.addr;

      serial::printf("vga: found framebuffer\n");
      serial::printf("vga: addr=0x%lx pitch=%u width=%u height=%u bpp=%u\n",
                     fb_info.addr, fb_info.pitch, fb_info.width, fb_info.height,
                     fb_info.bpp);
    }

    offset += tag_size;
    if (offset & 7) {
      offset = (offset + 7) & ~7;
    }
  }

  if (fb) {
    fb_fg.r = fb_fg.g = fb_fg.b = fb_fg.a = 0xFF;
    fb_bg.raw = 0;
  }

  if (!fb) {
    serial::printf("vga: no framebuffer found, using text mode fallback\n");
  }
}

void put_pixel(uint32_t x, uint32_t y, color c) {
  fb[y * (fb_info.pitch / 4) + x] = c.raw;
}

void put_char(uint32_t x, uint32_t y, char c, color fg, color bg) {
  if (!fb) {
    return;
  }
  for (int row = 0; row < 16; row++) {
    uint8_t bits = font::vga8x16[(uint8_t)c][row];
    for (int col = 0; col < 8; col++) {
      uint32_t px = x + col;
      uint32_t py = y + row;
      if (px >= fb_info.width || py >= fb_info.height) {
        continue;
      }
      if (bits & (0x80 >> col)) {
        fb[py * (fb_info.pitch / 4) + px] = fg.raw;
      } else {
        fb[py * (fb_info.pitch / 4) + px] = bg.raw;
      }
    }
  }
}

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

void __dep__write_char(char c) {
  if (c == '\n') {
    vga_idx.x = 0;
    vga_idx.y++;
    update_cursor();
    return;
  }

  if (c == '\b') {
    __dep__backspace();
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

void __dep__backspace() {
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

void __dep__write_string(const char *s) {
  for (int i = 0; s[i] != '\0'; i++) {
    __dep__write_char(s[i]);
  }
}

void __dep__printf(const char *format, ...) {
  char buf[1024];
  va_list args;
  va_start(args, format);
  utils::string::vsprintf(buf, format, args);
  va_end(args);
  __dep__write_string(buf);
}

void __dep__clear_scr() {
  for (size_t i = 0; i < 80 * 25; i++) {
    vga[i * 2] = ' ';
    vga[i * 2 + 1] = 0x0F;
  }

  vga_idx.x = vga_idx.y = 0;

  set_cursor_shape(0, 15);
  update_cursor();
}

void __dep__set_cursor_visible() {
  set_cursor_shape(0, 15);
  update_cursor();
}

static void fb_scroll() {
  uint32_t pitch32 = fb_info.pitch / 4;
  uint32_t rows = fb_info.height;
  uint32_t scroll_rows = 16;

  for (uint32_t y = 0; y < rows - scroll_rows; y++) {
    for (uint32_t x = 0; x < fb_info.width; x++) {
      fb[y * pitch32 + x] = fb[(y + scroll_rows) * pitch32 + x];
    }
  }

  for (uint32_t y = rows - scroll_rows; y < rows; y++) {
    for (uint32_t x = 0; x < fb_info.width; x++) {
      fb[y * pitch32 + x] = 0;
    }
  }
}

void write_char(char c) {
  if (!fb) {
    __dep__write_char(c);
    return;
  }

  uint32_t cols = fb_info.width / 8;
  uint32_t rows = fb_info.height / 16;

  if (c == '\n') {
    fb_cur.x = 0;
    fb_cur.y++;
    if (fb_cur.y >= rows) {
      fb_scroll();
      fb_cur.y = rows - 1;
    }
    return;
  }

  if (c == '\b') {
    backspace();
    return;
  }

  put_char(fb_cur.x * 8, fb_cur.y * 16, c, fb_fg, fb_bg);
  fb_cur.x++;

  if (fb_cur.x >= cols) {
    fb_cur.x = 0;
    fb_cur.y++;
    if (fb_cur.y >= rows) {
      fb_scroll();
      fb_cur.y = rows - 1;
    }
  }
}

void backspace() {
  if (!fb) {
    __dep__backspace();
    return;
  }

  uint32_t cols = fb_info.width / 8;

  if (fb_cur.x == 0 && fb_cur.y == 0) {
    return;
  }

  if (fb_cur.x == 0) {
    fb_cur.y--;
    fb_cur.x = cols - 1;
  } else {
    fb_cur.x--;
  }

  put_char(fb_cur.x * 8, fb_cur.y * 16, ' ', fb_fg, fb_bg);
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
  if (!fb) {
    __dep__clear_scr();
    return;
  }

  uint32_t pitch32 = fb_info.pitch / 4;
  for (uint32_t y = 0; y < fb_info.height; y++) {
    for (uint32_t x = 0; x < fb_info.width; x++) {
      fb[y * pitch32 + x] = 0;
    }
  }
  fb_cur.x = 0;
  fb_cur.y = 0;
}

void set_cursor_visible() {
}

} // namespace vga
