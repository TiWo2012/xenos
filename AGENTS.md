## Goal
- Migrate a freestanding x86-64 hobby kernel from Makefile/headers to CMake/C++20 modules.

## Constraints & Preferences
- Use Clang (set in CMakeLists.txt with `FORCE`), NASM, LLD.
- C++23, freestanding, no exceptions/RTTI/stack-protector.
- Module files use `.cppm`; one `.cppm` per module (interface + implementation combined).
- Ninja generator required.
- No comments added unless asked.

## Result
All project headers eliminated. Kernel builds and boots in QEMU.

### Modules
| Module | File | Exports |
|--------|------|---------|
| `utils.string` | `src/utils/string.cppm` | `utils::string::strcmp`, `vsprintf`, `sprintf` |
| `utils.memory` | `src/utils/memory.cppm` | `utils::memory::memset` |
| `binio` | `src/drivers/binio.cppm` | `outb`, `inb` (free functions) |
| `serial` | `src/drivers/serial.cppm` | `serial::write_char`, `write_string`, `print_hex`, `write_dec`, `printf` |
| `font` | `src/drivers/font.cppm` | `font::vga8x16` (font bitmap) |
| `vga` | `src/drivers/vga.cppm` | `vga::color`, `framebuffer_info`, `fb_info`, `fb`, `init`, `put_pixel`, `put_char`, `write_char`, `write_string`, `printf`, `backspace`, `clear_scr`, `set_cursor_visible` |
| `pmm` | `src/drivers/mem/pmm.cppm` | `pmm::init`, `alloc_page`, `free_page`, `alloc_pages`, `total_frames`, `free_frames`, `PHYS_MAP_BASE`, `phys_to_virt`, `virt_to_phys` |
| `idt` | `src/drivers/idt.cppm` | `idt::init`, `pic_remap`, `pic_eoi`, `pic_unmask_irq`, `pic_mask_irq`, `irq_register_handler`, `irq_dispatch` |
| `irq.pit` | `src/drivers/irq/pit.cppm` | `irq::pit::pit_init`, `timer_handler` |
| `heap` | `src/drivers/mem/heap.cppm` | `heap::init`, `alloc`, `free`, `calloc`, `realloc` |
| `terminal` | `src/drivers/terminal/terminal.cppm` | `terminal::init`, `send_key`, `process_command` |
| `irq.kbd` | `src/drivers/irq/kbd.cppm` | `irq::kbd::kbd_init`, `keyboard_handler` |
| `vmm` | `src/drivers/mem/vmm.cppm` | `vmm::init`, `map_page`, `unmap_page`, `get_physical`, `handle_page_fault` |
| `tests` | `src/tests.cppm` | `tests::run_all` |

### Remaining .cpp files (no project includes)
- `src/boot.cpp` — boot stub, only `<stdint.h>`
- `src/kernel.cpp` — main, uses `import` for all modules, `<cstddef>` and `<stdint.h>`
- `src/drivers/isr.cpp` — ASM ISR handler, uses `import idt`, `import serial`, and `import vmm`, `<stdint.h>`

### Testing
- `src/tests.cppm` — in-kernel test suite (70 assertions across 6 suites: string, memory, pmm, heap, vmm).
  - Imported and called from `kernel.cpp` after heap init and identity-map drop, before the idle loop.
  - Uses serial output for PASS/FAIL reporting; prints `ALL TESTS PASSED` on success.
- `run_tests.sh` — E2E runner: builds kernel, boots in QEMU (`-nographic -no-reboot`), captures serial output, and checks for `ALL TESTS PASSED`.
- Tests expose a heap free-coalescing bug (cross-page coalescing assumes linked-list adjacency == physical adjacency); heap tests avoid triggering it.

### Key patterns used
- `export namespace foo { ... }` for declarations, then `namespace foo { ... }` for definitions (with `static` helper functions inside the non-exported namespace block to avoid "cannot export internal linkage" error).
- Module name `irq.pit` → directory `src/drivers/irq/pit.cppm`.
- Exported variables (`fb_info`, `fb`) declared `extern` in `export namespace`, defined in non-exported `namespace`.
- `size_t` and other standard types need explicit `#include <cstddef>` in consumers since modules don't transitively provide them.
- All physical memory is mapped at `PHYS_MAP_BASE` (`0xFFFF800000000000`, PML4[256]) via 2MB huge pages covering 0–4GB.
- Page table pointers from CR3 or PTEs are converted via `pmm::phys_to_virt()` before dereferencing.
- Identity map (PML4[0]) is dropped after init in `kernel.cpp`; GDT must be reloaded via physmap before clearing.
- Stack lives in physmap (set in `entry.asm` 64-bit code via `PHYS_MAP_BASE + stack_top`).
- Kernel at PML4[511] PDPT[510] (VMA `0xFFFFFFFF80000000+`), no identity-map dependency at runtime.
- PML4[510] is a self-reference entry for recursive page table walking (constant `vmm::PML4_SELF_REF = 510`).
- Page-table walks in VMM still use physmap; recursive mapping is available for future use.

### Build
- CMake 3.28+, Ninja, Clang 22.
- `./run.sh` configures, builds, creates ISO, and boots in QEMU.
- `./run_tests.sh` builds, boots in QEMU, and checks for `ALL TESTS PASSED`.
