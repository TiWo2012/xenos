module;

#include <cstddef>
#include <cstdint>
#include <stdarg.h>

export module tests;

import utils;
import pmm;
import vmm;
import heap;
import serial;

export namespace tests {
void run_all();
} // namespace tests

namespace tests {

static int passed = 0;
static int failed = 0;

static void check(bool cond, const char *desc) {
  if (cond) {
    serial::printf("  PASS: %s\n", desc);
    passed++;
  } else {
    serial::printf("  FAIL: %s\n", desc);
    failed++;
  }
}

static bool buf_eq(const uint8_t *b, uint8_t v, size_t n) {
  for (size_t i = 0; i < n; i++)
    if (b[i] != v)
      return false;
  return true;
}

// ============================================================
// string::strcmp
// ============================================================

static void test_strcmp() {
  serial::printf("[string::strcmp]\n");
  check(string::strcmp("", "") == 0, "empty strings");
  check(string::strcmp("hello", "hello") == 0, "identical");
  check(string::strcmp("abc", "abd") < 0, "\"abc\" < \"abd\"");
  check(string::strcmp("abd", "abc") > 0, "\"abd\" > \"abc\"");
  check(string::strcmp("a", "") > 0, "non-empty > empty");
  check(string::strcmp("", "a") < 0, "empty < non-empty");
  check(string::strcmp("abc", "abcd") < 0, "prefix < longer");
  check(string::strcmp("abcd", "abc") > 0, "longer > prefix");
  check(string::strcmp("ABC", "abc") < 0, "case sensitive");
}

// ============================================================
// string::sprintf / vsprintf
// ============================================================

static void test_sprintf() {
  serial::printf("[string::sprintf]\n");
  char buf[256];

  check(string::sprintf(buf, "%d", 123) == 3 && string::strcmp(buf, "123") == 0, "%%d positive");
  check(string::sprintf(buf, "%d", 0) == 1 && string::strcmp(buf, "0") == 0, "%%d zero");

  check(string::sprintf(buf, "%u", 42) == 2 && string::strcmp(buf, "42") == 0, "%%u");
  check(string::sprintf(buf, "%u", 0) == 1 && string::strcmp(buf, "0") == 0, "%%u zero");
  check(string::sprintf(buf, "%u", 4294967295U) == 10 && string::strcmp(buf, "4294967295") == 0, "%%u max");

  check(string::sprintf(buf, "%x", 255) == 2 && string::strcmp(buf, "ff") == 0, "%%x");
  check(string::sprintf(buf, "%x", 0) == 1 && string::strcmp(buf, "0") == 0, "%%x zero");
  check(string::sprintf(buf, "%x", 0xABCD) == 4 && string::strcmp(buf, "abcd") == 0, "%%x abcd");

  check(string::sprintf(buf, "%lx", (unsigned long)0xdeadbeef) == 8 && string::strcmp(buf, "deadbeef") == 0, "%%lx");
  check(string::sprintf(buf, "%lx", (unsigned long)0) == 1 && string::strcmp(buf, "0") == 0, "%%lx zero");
  check(string::sprintf(buf, "%lx", (unsigned long)0xABCDEF12345678ULL) == 14 && string::strcmp(buf, "abcdef12345678") == 0, "%%lx large");

  check(string::sprintf(buf, "%s", "test") == 4 && string::strcmp(buf, "test") == 0, "%%s");
  check(string::sprintf(buf, "%s", "") == 0 && string::strcmp(buf, "") == 0, "%%s empty");

  check(string::sprintf(buf, "%c", 'A') == 1 && buf[0] == 'A' && buf[1] == '\0', "%%c");

  check(string::sprintf(buf, "%d %s 0x%x", 42, "hi", 0x1FF) == 11 && string::strcmp(buf, "42 hi 0x1ff") == 0, "%%d %%s %%x");

  // Note: kernel sprintf does not handle %% as literal %, it outputs %%
  check(string::sprintf(buf, "100%%") == 5 && string::strcmp(buf, "100%%") == 0, "literal %%%% -> %%%%");
  check(string::sprintf(buf, "hello world") == 11 && string::strcmp(buf, "hello world") == 0, "plain text");
  check(string::sprintf(buf, "a%db%sc", 1, "x") == 5 && string::strcmp(buf, "a1bxc") == 0, "interleaved %%d and %%s");

  int n = string::sprintf(buf, "abcdef");
  check(n == 6, "return value");
}

// ============================================================
// memory::memset
// ============================================================

static void test_memset() {
  serial::printf("[memory::memset]\n");

  uint8_t m[32];

  for (size_t i = 0; i < sizeof(m); i++) m[i] = 0xFF;
  memory::memset(m, 0, 10);
  check(buf_eq(m, 0, 10), "zero first 10");
  check(m[10] == 0xFF, "first untouched byte after zero");

  for (size_t i = 0; i < sizeof(m); i++) m[i] = 0;
  memory::memset(m, 0xAA, 5);
  check(buf_eq(m, 0xAA, 5), "0xAA first 5");
  check(m[5] == 0, "first untouched byte after AA");

  memory::memset(m, 0, sizeof(m));
  check(buf_eq(m, 0, sizeof(m)), "full zero");

  memory::memset(m, 0x5A, sizeof(m));
  check(buf_eq(m, 0x5A, sizeof(m)), "full 0x5A");

  memory::memset(m + 8, 0xFF, 8);
  check(buf_eq(m + 8, 0xFF, 8), "middle 8 bytes");
  check(m[7] == 0x5A && m[16] == 0x5A, "before and after middle write");
}

// ============================================================
// PMM
// ============================================================

static void test_pmm() {
  serial::printf("[pmm]\n");

  size_t free_before = pmm::free_frames();
  check(free_before > 0, "free frames > 0");

  void *p1 = pmm::alloc_page();
  check(p1 != nullptr, "alloc_page #1");
  check(((uintptr_t)p1 & 0xFFF) == 0, "page-aligned");

  void *p2 = pmm::alloc_page();
  check(p2 != nullptr, "alloc_page #2");
  check(p2 != p1, "distinct pages");
  check(pmm::free_frames() == free_before - 2, "free_frames -= 2");

  pmm::free_page(p1);
  pmm::free_page(p2);
  check(pmm::free_frames() == free_before, "free_frames restored");

  void *p3 = pmm::alloc_pages(3);
  check(p3 != nullptr, "alloc_pages(3)");
  check(((uintptr_t)p3 & 0xFFF) == 0, "3-page block aligned");
  check(pmm::free_frames() == free_before - 3, "free_frames -= 3");

  uint64_t base = (uint64_t)p3;
  pmm::free_page(p3);
  pmm::free_page((void *)(base + 0x1000));
  pmm::free_page((void *)(base + 0x2000));
  check(pmm::free_frames() == free_before, "free_frames restored after 3-page free");
}

// ============================================================
// VMM
// ============================================================

static void test_vmm() {
  serial::printf("[vmm]\n");

  void *phys = pmm::alloc_page();
  check(phys != nullptr, "phys page for mapping");

  void *virt = (void *)0xFFFF808000000000ULL;
  void *result = vmm::map_page(phys, virt, vmm::PAGE_PRESENT | vmm::PAGE_WRITE);
  check(result == virt, "map_page returns virt");

  void *phys2 = vmm::get_physical(virt);
  check(phys2 == phys, "get_physical roundtrip");

  volatile uint32_t *p = (volatile uint32_t *)virt;
  *p = 0xDEADBEEF;
  check(*p == 0xDEADBEEF, "write to mapped page");
  p[1] = 0x12345678;
  check(p[1] == 0x12345678, "write to mapped page +4");
  check(*p == 0xDEADBEEF, "first word unchanged");

  vmm::unmap_page(virt);
  check(vmm::get_physical(virt) == nullptr, "unmap clears mapping");

  pmm::free_page(phys);
}

// ============================================================
// Heap
// ============================================================

static void test_heap() {
  serial::printf("[heap]\n");

  void *p1 = heap::alloc(32);
  check(p1 != nullptr, "alloc(32)");
  volatile uint8_t *b = (volatile uint8_t *)p1;
  for (int i = 0; i < 32; i++)
    b[i] = (uint8_t)(i + 1);
  bool ok = true;
  for (int i = 0; i < 32; i++)
    if (b[i] != (uint8_t)(i + 1)) { ok = false; break; }
  check(ok, "write to alloc'd block");
  heap::free(p1);

  check(heap::alloc(0) == nullptr, "alloc(0)");

  void *a = heap::alloc(64);
  void *b2 = heap::alloc(64);
  void *c = heap::alloc(64);
  check(a && b2 && c, "3x alloc(64)");
  check(a != b2 && b2 != c && a != c, "distinct");
  heap::free(c);
  heap::free(b2);
  heap::free(a);

  void *z = heap::calloc(16, 4);
  check(z != nullptr, "calloc(16,4)");
  ok = true;
  for (int i = 0; i < 64; i++)
    if (((volatile uint8_t *)z)[i] != 0) { ok = false; break; }
  check(ok, "calloc zeroed");
  heap::free(z);

  void *rs = heap::alloc(64);
  check(rs != nullptr, "realloc shrink prep");
  for (int i = 0; i < 16; i++)
    ((volatile uint8_t *)rs)[i] = (uint8_t)('a' + i);
  void *rsh = heap::realloc(rs, 16);
  check(rsh == rs, "realloc shrink returns same ptr");
  ok = true;
  for (int i = 0; i < 16; i++)
    if (((volatile uint8_t *)rsh)[i] != (uint8_t)('a' + i)) { ok = false; break; }
  check(ok, "realloc shrink data preserved");
  heap::free(rsh);

  void *rn = heap::realloc(nullptr, 32);
  check(rn != nullptr, "realloc(nullptr,32)");
  heap::free(rn);

  void *rz = heap::alloc(32);
  check(rz != nullptr, "realloc(ptr,0) prep");
  void *rz2 = heap::realloc(rz, 0);
  check(rz2 == nullptr, "realloc(ptr,0)");

  void *ptrs[10];
  bool small_ok = true;
  for (int i = 0; i < 10; i++) {
    ptrs[i] = heap::alloc(8);
    if (!ptrs[i]) small_ok = false;
  }
  check(small_ok, "10x alloc(8)");
  for (int i = 0; i < 10; i++)
    heap::free(ptrs[i]);

  void *big = heap::alloc(2000);
  check(big != nullptr, "alloc(2000)");
  ok = true;
  for (int i = 0; i < 2000; i++)
    ((volatile uint8_t *)big)[i] = (uint8_t)(i & 0xFF);
  for (int i = 0; i < 2000; i++)
    if (((volatile uint8_t *)big)[i] != (uint8_t)(i & 0xFF)) { ok = false; break; }
  check(ok, "large block writable");
  heap::free(big);
}

// ============================================================
// Runner
// ============================================================

export void run_all() {
  serial::printf("\n========================================\n");
  serial::printf("         RUNNING ALL TESTS\n");
  serial::printf("========================================\n");

  test_strcmp();
  test_sprintf();
  test_memset();
  test_pmm();
  test_heap();
  test_vmm();

  serial::printf("\n========================================\n");
  serial::printf("  Ran: %d  |  Passed: %d  |  Failed: %d\n",
                 passed + failed, passed, failed);
  serial::printf("========================================\n");

  if (failed == 0) {
    serial::printf("  *** ALL TESTS PASSED ***\n");
  } else {
    serial::printf("  *** SOME TESTS FAILED ***\n");
  }
  serial::printf("========================================\n\n");
}

} // namespace tests
