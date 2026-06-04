#pragma once

#include <cstdint>
#include <cstddef>

namespace pmm {

void init(uint32_t mb_info);
void* alloc_page();
void free_page(void* phys);
void* alloc_pages(size_t n);

size_t total_frames();
size_t free_frames();

}
