#include "string.h"
#include <cstdarg>
#include <stdarg.h>

namespace utils {
namespace string {
int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return (unsigned char)*s1 - (unsigned char)*s2;
}

int vsprintf(char *s, const char *format, va_list args) {
  char *out = s;

  for (const char *f = format; *f; f++) {
    if (*f != '%') {
      *out++ = *f;
      continue;
    }

    f++; // skip '%'

    if (*f == 'd' || *f == 'u') {
      unsigned int val;
      if (*f == 'd')
        val = (unsigned int)va_arg(args, int);
      else
        val = va_arg(args, unsigned int);
      char buf[32];
      int i = 0;
      if (val == 0) {
        buf[i++] = '0';
      } else {
        while (val > 0) {
          buf[i++] = '0' + (val % 10);
          val /= 10;
        }
      }
      // reverse
      for (int j = i - 1; j >= 0; j--) {
        *out++ = buf[j];
      }

    } else if (*f == 'x') {
      unsigned int val = va_arg(args, unsigned int);
      char buf[32];
      int i = 0;
      if (val == 0) {
        buf[i++] = '0';
      } else {
        while (val > 0) {
          int digit = val & 0xF;
          buf[i++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
          val >>= 4;
        }
      }
      for (int j = i - 1; j >= 0; j--) {
        *out++ = buf[j];
      }

    } else if (*f == 'l' && *(f+1) == 'x') {
      f++;
      unsigned long val = va_arg(args, unsigned long);
      char buf[32];
      int i = 0;
      if (val == 0) {
        buf[i++] = '0';
      } else {
        while (val > 0) {
          int digit = val & 0xF;
          buf[i++] = digit < 10 ? '0' + digit : 'a' + digit - 10;
          val >>= 4;
        }
      }
      for (int j = i - 1; j >= 0; j--) {
        *out++ = buf[j];
      }

    } else if (*f == 's') {
      char *str = va_arg(args, char *);
      while (*str) {
        *out++ = *str++;
      }

    } else if (*f == 'c') {
      char c = (char)va_arg(args, int);
      *out++ = c;

    } else {
      // unknown specifier, just print it
      *out++ = '%';
      *out++ = *f;
    }
  }

  *out = '\0';
  return out - s; // number of chars written
}

int sprintf(char *s, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int ret = vsprintf(s, format, args);
  va_end(args);
  return ret;
}

} // namespace string
} // namespace utils
