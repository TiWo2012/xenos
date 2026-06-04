#pragma once

#include <stdarg.h>

namespace utils {
namespace string {
int strcmp(const char *s1, const char *s2);
int vsprintf(char *s, const char *format, va_list args);
int sprintf(char *s, const char *format, ...);
}
} // namespace utils
