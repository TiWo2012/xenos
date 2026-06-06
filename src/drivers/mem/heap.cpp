#include "heap.h"
import serial;
import utils.memory;
#include "pmm.h"

namespace heap {

static const size_t BLOCK_HEADER_SIZE = 32;
static const size_t MIN_SPLIT = BLOCK_HEADER_SIZE + 16;

struct block {
  size_t size;
  bool free;
  block *prev;
  block *next;
};

static block *head = nullptr;

void init(size_t size) {
  serial::printf("heap: requesting %u bytes from pmm\n", (uint32_t)size);
  size_t num_pages = (size + 4095) / 4096;
  void *base = nullptr;
  while (num_pages > 0) {
    base = pmm::alloc_pages(num_pages);
    if (base) {
      break;
    }
    num_pages--;
  }
  if (!base) {
    serial::printf("heap: ERROR - pmm out of memory\n");
    return;
  }

  size_t actual_size = num_pages * 4096;
  for (size_t i = 0; i < actual_size; i += 4) {
    *(volatile uint32_t *)((uintptr_t)base + i) = 0;
  }

  head = (block *)base;
  head->size = actual_size;
  head->free = true;
  head->prev = nullptr;
  head->next = nullptr;

  serial::printf("heap: init done, base=0x%lx, size=%u (%u pages)\n",
                 (uint64_t)head, (uint32_t)actual_size, (uint32_t)num_pages);
}

void *alloc(size_t size) {
  if (size == 0) {
    return nullptr;
  }

  size = (size + 15) & ~15;

  block *cur = head;
  while (cur) {
    if (cur->free) {
      size_t avail = cur->size - BLOCK_HEADER_SIZE;
      if (avail >= size) {
        size_t leftover = avail - size;
        if (leftover >= MIN_SPLIT) {
          block *nb = (block *)((uint8_t *)cur + BLOCK_HEADER_SIZE + size);
          nb->size = leftover;
          nb->free = true;
          nb->prev = cur;
          nb->next = cur->next;
          if (cur->next) {
            cur->next->prev = nb;
          }
          cur->next = nb;
          cur->size = BLOCK_HEADER_SIZE + size;
        }
        cur->free = false;
        return (uint8_t *)cur + BLOCK_HEADER_SIZE;
      }
    }
    cur = cur->next;
  }

  serial::printf("heap: OOM (requested %u)\n", size);
  return nullptr;
}

void free(void *ptr) {
  if (!ptr) {
    return;
  }

  block *b = (block *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);
  b->free = true;

  if (b->next && b->next->free) {
    b->size += b->next->size;
    b->next = b->next->next;
    if (b->next) {
      b->next->prev = b;
    }
  }

  if (b->prev && b->prev->free) {
    b->prev->size += b->size;
    b->prev->next = b->next;
    if (b->next) {
      b->next->prev = b->prev;
    }
  }
}

void *calloc(size_t num, size_t size) {
  size_t total = num * size;
  void *p = alloc(total);
  if (p) {
    utils::memory::memset(p, 0, total);
  }
  return p;
}

void *realloc(void *ptr, size_t new_size) {
  if (!ptr) {
    return alloc(new_size);
  }
  if (new_size == 0) {
    free(ptr);
    return nullptr;
  }

  block *b = (block *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);
  size_t old_size = b->size - BLOCK_HEADER_SIZE;

  if (old_size >= new_size) {
    return ptr;
  }

  void *new_ptr = alloc(new_size);
  if (!new_ptr) {
    return nullptr;
  }

  utils::memory::memset(new_ptr, 0, new_size);
  for (size_t i = 0; i < old_size; i++) {
    ((uint8_t *)new_ptr)[i] = ((uint8_t *)ptr)[i];
  }

  free(ptr);
  return new_ptr;
}

} // namespace heap
