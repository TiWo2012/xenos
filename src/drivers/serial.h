#pragma once

#include <cstdint>
namespace serial {

void init();
void write_char(char c);
void write_string(const char *s);
void print_hex(uint64_t val);
void write_dec(uint64_t val);
void printf(const char *format, ...);

} // namespace serial
