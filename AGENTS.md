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
| `pmm` | `src/drivers/mem/pmm.cppm` | `pmm::init`, `alloc_page`, `free_page`, `alloc_pages`, `total_frames`, `free_frames` |
| `idt` | `src/drivers/idt.cppm` | `idt::init`, `pic_remap`, `pic_eoi`, `pic_unmask_irq`, `pic_mask_irq`, `irq_register_handler`, `irq_dispatch` |
| `irq.pit` | `src/drivers/irq/pit.cppm` | `irq::pit::pit_init`, `timer_handler` |
| `heap` | `src/drivers/mem/heap.cppm` | `heap::init`, `alloc`, `free`, `calloc`, `realloc` |
| `terminal` | `src/drivers/terminal/terminal.cppm` | `terminal::init`, `send_key`, `process_command` |
| `irq.kbd` | `src/drivers/irq/kbd.cppm` | `irq::kbd::kbd_init`, `keyboard_handler` |
| `vmm` | `src/drivers/mem/vmm.cppm` | `vmm::init`, `map_page`, `unmap_page`, `get_physical`, `handle_page_fault` |

### Remaining .cpp files (no project includes)
- `src/boot.cpp` — boot stub, only `<stdint.h>`
- `src/kernel.cpp` — main, uses `import` for all modules, `<cstddef>` and `<stdint.h>`
- `src/drivers/isr.cpp` — ASM ISR handler, uses `import idt`, `import serial`, and `import vmm`, `<stdint.h>`

### Key patterns used
- `export namespace foo { ... }` for declarations, then `namespace foo { ... }` for definitions (with `static` helper functions inside the non-exported namespace block to avoid "cannot export internal linkage" error).
- Module name `irq.pit` → directory `src/drivers/irq/pit.cppm`.
- Exported variables (`fb_info`, `fb`) declared `extern` in `export namespace`, defined in non-exported `namespace`.
- `size_t` and other standard types need explicit `#include <cstddef>` in consumers since modules don't transitively provide them.

### Build
- CMake 3.28+, Ninja, Clang 22.
- `./run.sh` configures, builds, creates ISO, and boots in QEMU.
