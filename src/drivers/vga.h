#pragma once

#include <stdint.h>

namespace vga {

union color {
  uint32_t raw;
  struct {
    uint8_t b;
    uint8_t g;
    uint8_t r;
    uint8_t a;
  };
};

struct framebuffer_info {
  uint64_t addr;
  uint32_t pitch;
  uint32_t width;
  uint32_t height;
  uint8_t bpp;
};

extern framebuffer_info fb_info;
extern volatile uint32_t* fb;

void init(uint32_t mb_info_addr);
void put_pixel(uint32_t x, uint32_t y, color c);
void put_char(uint32_t x, uint32_t y, char c, color fg, color bg);

void __dep__write_char(char c);
void __dep__write_string(const char *s);
void __dep__printf(const char *format, ...);
void __dep__backspace();
void __dep__clear_scr();
void __dep__set_cursor_visible();

void write_char(char c);
void write_string(const char *s);
void printf(const char *format, ...);
void backspace();
void clear_scr();
void set_cursor_visible();
} // namespace vga
