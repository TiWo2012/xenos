#include "memory.h"

namespace utils {
namespace memory {

void *memset(void *p, int v, int n) {
  unsigned char *ptr = (unsigned char *)p;

  while (n--) {
    *ptr++ = (unsigned char)v;
  }

  return p;
}

} // namespace memory
} // namespace utils
