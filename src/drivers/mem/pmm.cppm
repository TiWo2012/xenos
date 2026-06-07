module;

#include <cstdint>
#include <cstddef>

export module pmm;

import serial;

extern "C" uintptr_t KERNEL_START;
extern "C" uintptr_t KERNEL_END;

static constexpr uint64_t KERNEL_OFFSET = 0xFFFFFFFF80000000ULL;

export namespace pmm {

void init(uint32_t mb_info);
void* alloc_page();
void free_page(void* phys);
void* alloc_pages(size_t n);

size_t total_frames();
size_t free_frames();

} // namespace pmm

namespace pmm {

static const size_t MAX_FRAMES = 0x100000;
static const size_t BITMAP_SIZE = MAX_FRAMES / 8;

static uint8_t bitmap[BITMAP_SIZE] __attribute__((aligned(4096)));
static size_t last_alloc = 0;
static size_t used_frames = 0;

static inline size_t frame_idx(uint64_t paddr) {
  return paddr / 4096;
}

static inline void set_bit(size_t i) {
  bitmap[i / 8] |= (1 << (i % 8));
}

static inline void clear_bit(size_t i) {
  bitmap[i / 8] &= ~(1 << (i % 8));
}

static inline bool test_bit(size_t i) {
  return (bitmap[i / 8] >> (i % 8)) & 1;
}

static void mark_used(uint64_t start, uint64_t end) {
  start &= ~0xFFF;
  end = (end + 0xFFF) & ~0xFFF;
  for (uint64_t p = start; p < end; p += 4096) {
    size_t i = frame_idx(p);
    if (i < MAX_FRAMES && !test_bit(i)) {
      set_bit(i);
      used_frames++;
    }
  }
}

struct multiboot_tag {
  uint32_t type;
  uint32_t size;
};

struct mmap_tag {
  uint32_t type;
  uint32_t size;
  uint32_t entry_size;
  uint32_t entry_version;
};

struct mmap_entry {
  uint64_t base_addr;
  uint64_t length;
  uint32_t type;
  uint32_t reserved;
};

void init(uint32_t mb_info_addr) {
  serial::printf("pmm: init, bitmap at 0x%lx (phys 0x%lx), size %u\n",
                 (uint64_t)bitmap, (uint64_t)bitmap - KERNEL_OFFSET, BITMAP_SIZE);

  for (size_t i = 0; i < BITMAP_SIZE; i++) {
    bitmap[i] = 0xFF;
  }
  used_frames = MAX_FRAMES;

  uint8_t* mb = (uint8_t*)(uint64_t)mb_info_addr;
  uint32_t total_size = *(uint32_t*)mb;
  uintptr_t kernel_start = (uintptr_t)&KERNEL_START - KERNEL_OFFSET;
  uintptr_t kernel_end = (uintptr_t)&KERNEL_END - KERNEL_OFFSET;

  serial::printf("pmm: kernel 0x%lx - 0x%lx (phys 0x%lx - 0x%lx), mb_info at 0x%lx, total_size %u\n",
                 (uintptr_t)&KERNEL_START, (uintptr_t)&KERNEL_END,
                 kernel_start, kernel_end, (uint64_t)mb, total_size);

  uint32_t offset = 8;
  while (offset + 8 <= total_size) {
    multiboot_tag* tag = (multiboot_tag*)(mb + offset);
    if (tag->type == 0) {
      break;
    }
    if (tag->type == 6 && tag->size >= sizeof(mmap_tag)) {
      mmap_tag* mt = (mmap_tag*)tag;
      uint32_t entry_count = (mt->size - sizeof(mmap_tag)) / mt->entry_size;
      mmap_entry* entries = (mmap_entry*)(mb + offset + sizeof(mmap_tag));
      uint64_t total_ram = 0;
      serial::printf("pmm: mmap has %u entries\n", entry_count);
      for (uint32_t i = 0; i < entry_count; i++) {
        mmap_entry* e = (mmap_entry*)((uint8_t*)entries + i * mt->entry_size);
        uint64_t end = e->base_addr + e->length;
        serial::printf("  mmap[%u] base=0x%lx len=0x%lx type=%u\n",
                       i, e->base_addr, e->length, e->type);
        if (e->type == 1) {
          total_ram += e->length;
          for (uint64_t p = e->base_addr; p < end && p < 0x100000000ULL; p += 4096) {
            size_t fi = frame_idx(p);
            if (fi < MAX_FRAMES && test_bit(fi)) {
              clear_bit(fi);
              used_frames--;
            }
          }
        }
      }
      serial::printf("pmm: total installed memory: %u MB (%u bytes)\n",
                     (uint32_t)(total_ram / (1024 * 1024)), (uint32_t)total_ram);
    }
    offset += tag->size;
    if (offset & 7) {
      offset = (offset + 7) & ~7;
    }
  }

  mark_used(0, 0x1000);
  mark_used(0x100000, kernel_start);  // .boot section (page tables, stack, code at 1M+)
  mark_used(kernel_start, kernel_end);
  mark_used((uint64_t)bitmap - KERNEL_OFFSET, (uint64_t)bitmap - KERNEL_OFFSET + BITMAP_SIZE);
  mark_used((uint64_t)mb, (uint64_t)mb + total_size);

  serial::printf("pmm: done, free frames: %u\n", MAX_FRAMES - used_frames);
}

void* alloc_page() {
  for (size_t i = last_alloc; i < MAX_FRAMES; i++) {
    if (!test_bit(i)) {
      set_bit(i);
      used_frames++;
      last_alloc = i;
      return (void*)(i * 4096);
    }
  }
  for (size_t i = 0; i < last_alloc; i++) {
    if (!test_bit(i)) {
      set_bit(i);
      used_frames++;
      last_alloc = i;
      return (void*)(i * 4096);
    }
  }
  return nullptr;
}

void free_page(void* phys) {
  uint64_t addr = (uint64_t)phys;
  if (addr & 0xFFF) {
    return;
  }
  size_t i = frame_idx(addr);
  if (i < MAX_FRAMES && test_bit(i)) {
    clear_bit(i);
    used_frames--;
    if (i < last_alloc) {
      last_alloc = i;
    }
  }
}

void* alloc_pages(size_t n) {
  if (n == 0) {
    return nullptr;
  }
  if (n == 1) {
    return alloc_page();
  }

  for (size_t start = last_alloc; start + n <= MAX_FRAMES; start++) {
    bool ok = true;
    for (size_t j = 0; j < n; j++) {
      if (test_bit(start + j)) {
        ok = false;
        break;
      }
    }
    if (!ok) {
      continue;
    }
    for (size_t j = 0; j < n; j++) {
      set_bit(start + j);
    }
    used_frames += n;
    last_alloc = start + n;
    return (void*)(start * 4096);
  }

  for (size_t start = 0; start + n <= last_alloc; start++) {
    bool ok = true;
    for (size_t j = 0; j < n; j++) {
      if (test_bit(start + j)) {
        ok = false;
        break;
      }
    }
    if (!ok) {
      continue;
    }
    for (size_t j = 0; j < n; j++) {
      set_bit(start + j);
    }
    used_frames += n;
    last_alloc = start + n;
    return (void*)(start * 4096);
  }
  return nullptr;
}

size_t total_frames() { return MAX_FRAMES; }
size_t free_frames() { return MAX_FRAMES - used_frames; }

} // namespace pmm
