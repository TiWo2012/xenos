#pragma once

namespace vga {
void write_char(char c);
void write_string(const char *s);
void backspace();
void clear_scr();
} // namespace vga
