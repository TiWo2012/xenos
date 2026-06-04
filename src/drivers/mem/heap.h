#pragma once

#include <cstdint>
#include <cstddef>

namespace heap {

void init();
void* alloc(size_t size);
void free(void* ptr);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t new_size);

}
