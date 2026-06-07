module;

#include <cstdint>
#include <cstddef>

export module vmm;

import pmm;
import serial;
import utils.memory;

export namespace vmm {

constexpr uint64_t PAGE_PRESENT = 0x1;
constexpr uint64_t PAGE_WRITE = 0x2;
constexpr uint64_t PAGE_USER = 0x4;
constexpr uint64_t PAGE_HUGE = 0x80;

constexpr uint16_t PT_ENTRIES = 512;

void init();
void* map_page(void* phys, void* virt, uint64_t flags);
void unmap_page(void* virt);
void* get_physical(void* virt);
void* map_contiguous(void* phys, void* virt, size_t pages, uint64_t flags);

void handle_page_fault(uint64_t addr, uint64_t error_code);

} // namespace vmm

namespace vmm {

static inline uint16_t pml4_index(uint64_t v) { return (v >> 39) & 0x1FF; }
static inline uint16_t pdpt_index(uint64_t v) { return (v >> 30) & 0x1FF; }
static inline uint16_t pd_index(uint64_t v)   { return (v >> 21) & 0x1FF; }
static inline uint16_t pt_index(uint64_t v)   { return (v >> 12) & 0x1FF; }

struct page_table {
  uint64_t entries[512];
} __attribute__((aligned(4096)));

static page_table* get_pml4() {
  uint64_t cr3;
  __asm__ __volatile__("mov %%cr3, %0" : "=r"(cr3));
  return (page_table*)pmm::phys_to_virt(cr3);
}

static void invlpg(void* v) {
  __asm__ __volatile__("invlpg (%0)" : : "r"(v) : "memory");
}

void init() {
  serial::printf("vmm: init, pml4 at 0x%lx\n", (uint64_t)get_pml4());
}

void* map_page(void* phys, void* virt, uint64_t flags) {
  uint64_t vaddr = (uint64_t)virt;
  uint64_t paddr = (uint64_t)phys;

  page_table* pml4 = get_pml4();

  uint16_t i4 = pml4_index(vaddr);
  uint16_t i3 = pdpt_index(vaddr);
  uint16_t i2 = pd_index(vaddr);
  uint16_t i1 = pt_index(vaddr);

  uint64_t e = pml4->entries[i4];
  if (!(e & PAGE_PRESENT)) {
    page_table* p = (page_table*)pmm::phys_to_virt((uint64_t)pmm::alloc_page());
    if (!p) return nullptr;
    utils::memory::memset(p, 0, 4096);
    pml4->entries[i4] = pmm::virt_to_phys(p) | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);
  }
  page_table* p3 = (page_table*)pmm::phys_to_virt(pml4->entries[i4] & ~0xFFF);

  e = p3->entries[i3];
  if (!(e & PAGE_PRESENT)) {
    page_table* p = (page_table*)pmm::phys_to_virt((uint64_t)pmm::alloc_page());
    if (!p) return nullptr;
    utils::memory::memset(p, 0, 4096);
    p3->entries[i3] = pmm::virt_to_phys(p) | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);
  }
  page_table* p2 = (page_table*)pmm::phys_to_virt(p3->entries[i3] & ~0xFFF);

  e = p2->entries[i2];
  if (!(e & PAGE_PRESENT)) {
    page_table* p = (page_table*)pmm::phys_to_virt((uint64_t)pmm::alloc_page());
    if (!p) return nullptr;
    utils::memory::memset(p, 0, 4096);
    p2->entries[i2] = pmm::virt_to_phys(p) | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);
  } else if (e & PAGE_HUGE) {
    page_table* pt = (page_table*)pmm::phys_to_virt((uint64_t)pmm::alloc_page());
    if (!pt) return nullptr;
    uint64_t base = (e & ~0x1FFFFF);
    uint64_t pt_flags = (e & ~(PAGE_HUGE | 0x1FFFFF)) | PAGE_PRESENT;
    for (int i = 0; i < 512; i++) {
      pt->entries[i] = (base + (uint64_t)i * 4096) | pt_flags;
    }
    p2->entries[i2] = pmm::virt_to_phys(pt) | PAGE_PRESENT | PAGE_WRITE | (flags & PAGE_USER);
  }
  page_table* p1 = (page_table*)pmm::phys_to_virt(p2->entries[i2] & ~0xFFF);

  p1->entries[i1] = paddr | flags | PAGE_PRESENT;
  invlpg(virt);
  return virt;
}

void* map_contiguous(void* phys, void* virt, size_t pages, uint64_t flags) {
  uint64_t p = (uint64_t)phys;
  uint64_t v = (uint64_t)virt;
  for (size_t i = 0; i < pages; i++) {
    if (!map_page((void*)p, (void*)v, flags)) {
      for (size_t j = 0; j < i; j++) {
        unmap_page((void*)((uint64_t)virt + j * 4096));
      }
      return nullptr;
    }
    p += 4096;
    v += 4096;
  }
  return virt;
}

void unmap_page(void* virt) {
  uint64_t vaddr = (uint64_t)virt;
  page_table* pml4 = get_pml4();

  uint64_t e = pml4->entries[pml4_index(vaddr)];
  if (!(e & PAGE_PRESENT)) return;
  page_table* p3 = (page_table*)pmm::phys_to_virt(e & ~0xFFF);

  e = p3->entries[pdpt_index(vaddr)];
  if (!(e & PAGE_PRESENT)) return;
  page_table* p2 = (page_table*)pmm::phys_to_virt(e & ~0xFFF);

  e = p2->entries[pd_index(vaddr)];
  if (!(e & PAGE_PRESENT) || (e & PAGE_HUGE)) return;
  page_table* p1 = (page_table*)pmm::phys_to_virt(e & ~0xFFF);

  p1->entries[pt_index(vaddr)] = 0;
  invlpg(virt);
}

void* get_physical(void* virt) {
  uint64_t vaddr = (uint64_t)virt;
  page_table* pml4 = get_pml4();

  uint64_t e = pml4->entries[pml4_index(vaddr)];
  if (!(e & PAGE_PRESENT)) return nullptr;
  page_table* p3 = (page_table*)pmm::phys_to_virt(e & ~0xFFF);

  e = p3->entries[pdpt_index(vaddr)];
  if (!(e & PAGE_PRESENT)) return nullptr;
  page_table* p2 = (page_table*)pmm::phys_to_virt(e & ~0xFFF);

  e = p2->entries[pd_index(vaddr)];
  if (!(e & PAGE_PRESENT)) return nullptr;

  if (e & PAGE_HUGE) {
    return (void*)((e & ~0x1FFFFF) | (vaddr & 0x1FFFFF));
  }
  page_table* p1 = (page_table*)pmm::phys_to_virt(e & ~0xFFF);

  e = p1->entries[pt_index(vaddr)];
  if (!(e & PAGE_PRESENT)) return nullptr;
  return (void*)((e & ~0xFFF) | (vaddr & 0xFFF));
}

void handle_page_fault(uint64_t addr, uint64_t error_code) {
  serial::printf("\nPAGE FAULT at 0x%lx  error=0x%lx\n", addr, error_code);
  serial::printf("  P=%d W=%d U=%d R=%d I=%d PK=%d\n",
    (int)(error_code & 1),
    (int)((error_code >> 1) & 1),
    (int)((error_code >> 2) & 1),
    (int)((error_code >> 3) & 1),
    (int)((error_code >> 4) & 1),
    (int)((error_code >> 5) & 1));
  __asm__ __volatile__("cli; hlt");
}

} // namespace vmm
