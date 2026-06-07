# Xenos

A hobby x86-64 operating system kernel written from scratch in C++23 (using C++20 modules) and NASM assembly. Boots via GRUB (Multiboot2), runs in long mode with 4-level paging, and provides an interactive shell over VGA text mode.

## Features

- **C++20 modules** — fully migrated from headers; each driver is a standalone `.cppm` module
- **Multiboot2** — boots via GRUB with standard header
- **Long mode (64-bit)** — 4-level paging with 2 MB huge pages, identity-mapping the first 1 GB
- **GDT** — kernel code (ring 0) and data segments
- **IDT** — 34 entries (CPU exceptions 0–31 + IRQ0/IRQ1), interrupt gates, assembly ISR stubs with full register save/restore
- **PIC (8259A)** — remapped master to vector 32, slave to vector 40, 8086 mode
- **Exceptions** — full register dump (GPRs, RIP, CS, RFLAGS, RSP, GDTR) to serial on any CPU exception
- **PIT timer** — 1000 Hz, tick counter
- **PS/2 keyboard** — scancode set 1, US ANSI layout, IRQ1-driven
- **Serial (COM1)** — 38400 baud 8N1, loopback-tested, `printf` support
- **VGA text mode** — 80×25, hardware cursor, `printf` support
- **Physical Memory Manager (PMM)** — bitmap allocator, parses GRUB memory map, tracks used/free frames
- **Kernel heap** — linked-list dynamic allocator with splitting, coalescing, `calloc`/`realloc`
- **Terminal shell** — interactive command line over VGA with keyboard input; commands: `clear`, `exit` (ACPI shutdown), `mem` (PMM stats)
- **ACPI shutdown** — power-off via PM1a_CNT port (`0x604`)
- **Formatted output** — `printf`/`vsprintf` with `%d`, `%u`, `%x`, `%lx`, `%s`, `%c`

## Prerequisites

- clang++ (C++23)
- cmake (3.28+)
- ninja
- nasm
- ld.lld
- grub-mkrescue
- qemu-system-x86_64

## Build & Run

```sh
./run.sh   # configure (Ninja), build, and boot in QEMU
```

The kernel boots in QEMU with the serial port connected to stdio, so all serial debug output appears in your terminal. The VGA console is also visible in the QEMU window.

## Project Structure

```
├── CMakeLists.txt              # Build system (CMake + Ninja, clang/lld/nasm)
├── run.sh                      # One-step configure, build, boot
├── linker.ld                   # Linker script (kernel at 1M physical)
├── grub.cfg                    # GRUB boot menu config source
├── test.py                     # Generate ISR extern declarations
├── src/
│   ├── entry.asm               # Multiboot2 header, long-mode init, paging, GDT
│   ├── boot.cpp                # C++ wrapper: calls kernel_main (no imports)
│   ├── kernel.cpp              # Kernel init: imports all modules, calls init sequence
│   ├── drivers/
│   │   ├── binio.cppm          # Module `binio`: port I/O (inb/outb)
│   │   ├── font.cppm           # Module `font`: VGA 8×16 font bitmap
│   │   ├── vga.cppm            # Module `vga`: VGA text-mode driver (80×25, cursor, printf)
│   │   ├── serial.cppm         # Module `serial`: COM1 driver (38400 8N1, printf)
│   │   ├── idt.cppm            # Module `idt`: IDT setup, PIC management, IRQ dispatch
│   │   ├── idtl.asm            # Assembly ISR stubs (32 hand-written), load_idt, iretq test
│   │   ├── isr.cpp             # C++ exception handler with full register dump (imports idt, serial)
│   │   ├── irq/
│   │   │   ├── pit.cppm        # Module `irq.pit`: PIT timer (1000 Hz)
│   │   │   └── kbd.cppm        # Module `irq.kbd`: PS/2 keyboard (scancode set 1, US ANSI)
│   │   ├── terminal/
│   │   │   ├── terminal.cppm   # Module `terminal`: interactive shell (clear, exit, mem)
│   │   │   └── asm_shutdown.asm# ACPI power-off
│   │   └── mem/
│   │       ├── pmm.cppm        # Module `pmm`: physical memory manager (bitmap, 4 GB max)
│   │       └── heap.cppm       # Module `heap`: kernel heap (linked-list, split, coalesce)
│   └── utils/
│       ├── memory.cppm         # Module `utils.memory`: memset (freestanding)
│       └── string.cppm         # Module `utils.string`: strcmp, vsprintf, sprintf
```

## Architecture

### Boot Sequence

1. GRUB loads `kernel.elf` at physical address `0x100000` (1 MB) via the Multiboot2 header defined in `entry.asm`.
2. `_start` (32-bit) disables interrupts, zeroes page tables, identity-maps the first 1 GB with 2 MB huge pages, enables PAE, loads PML4 into CR3, sets EFER.LME for long mode, enables paging, loads the GDT, and far-jumps into 64-bit mode.
3. `long_mode_entry` sets segment registers, restores the multiboot magic and info pointer, and calls `boot_main(magic, mb_info)`.
4. `boot_main` forwards to `kernel_main`.

### Initialization Order

In `kernel_main`:
1. Clear VGA screen
2. Remap PIC (master → vector 32, slave → vector 40)
3. Load IDT (entries 0–33: exceptions + IRQ0/IRQ1)
4. Initialize PIT at 1000 Hz, register timer handler for IRQ0
5. Initialize keyboard, register keyboard handler for IRQ1
6. Unmask IRQ0 and IRQ1; enable interrupts (`sti`)
7. Run iretq self-test
8. Initialize PMM from multiboot memory map
9. Print welcome banner, initialize terminal shell
10. Initialize heap using all free physical memory
11. Enter infinite `hlt` loop

### Memory Map

| Region | Contents |
|---|---|
| `0x00000000` – `0x00000FFF` | Reserved (PMM marks first page used) |
| `0x100000` – `KERNEL_END` | Kernel .text, .rodata, .data, .bss |
| (after kernel) | PMM bitmap (aligned 4K) |
| (after bitmap) | Multiboot info / memory map |
| (everything else) | Free — managed by PMM, backing kernel heap |

### Interrupt Handling

- **IDT**: 256-entry table, gate type interrupt (0x8E), ring 0, kernel code selector 0x08
- **ISR stubs**: Each entry pushes an error code (fake if none), pushes the interrupt number, saves all 15 GPRs, calls `isr_handler()`, restores, and `iretq`
- **Dispatch**: For IRQs (int_no ≥ 32), `isr_handler` → `irq_dispatch` → `pic_eoi` + registered handler
- **Exceptions**: Full register state (R15–RAX, int_no, error_code) plus RIP, CS, RFLAGS, RSP, and GDTR are dumped to serial

### Physical Memory Manager

- Bitmap with 1,048,576 entries covering 4 GB of address space
- All memory starts as "used"; the multiboot memory map (tag type 6) marks available RAM entries as "free"
- Reserved: first physical page, kernel image, PMM bitmap, multiboot structures
- `alloc_page` does a linear scan with a `last_alloc` hint; `alloc_pages` finds contiguous runs

### Heap Allocator

- Linked-list of `block` headers (`size`, `free`, `prev`, `next`)
- First-fit allocation with splitting if the leftover ≥ 48 bytes
- `free` coalesces with adjacent free blocks
- Backed by PMM pages; tries multiple sizes if the initial allocation fails
- Supports `calloc` (zero-initialised) and `realloc` (grow/shrink with copy)

### Terminal / Shell

- VGA text-mode prompt (`"> "`) with keyboard echo
- 256-byte command buffer, backspace and Enter support
- **Commands:**
  - `clear` — clear the VGA screen
  - `exit` — ACPI system power-off via `outw 0x2000, 0x604`
  - `mem` — print total, free, and used physical frames to VGA and serial
  - `*` — anything else prints "command does not exist"

## Build System

The project uses **CMake** (3.28+) with the **Ninja** generator, Clang, NASM, and LLD.

- Module sources (`.cppm`) are registered as a `FILE_SET CXX_MODULES` in `CMakeLists.txt`, enabling Clang's module support
- `./run.sh` runs `cmake --fresh -B build -G Ninja` then `cmake --build build --target run`
- `os.iso` is produced by `grub-mkrescue` via a custom CMake target
- QEMU is invoked with `-cdrom os.iso -serial stdio`

## Module API

### `binio`
```cpp
import binio;
outb(port, val); inb(port);
```

### `vga`
```cpp
import vga;
vga::write_char, vga::write_string, vga::printf;
vga::backspace, vga::clear_scr, vga::set_cursor_visible;
```

### `serial`
```cpp
import serial;
serial::printf, serial::write_char, serial::write_string;
serial::print_hex, serial::write_dec;
```

### `idt`
```cpp
import idt;
idt::init, idt::pic_remap, idt::pic_eoi;
idt::irq_register_handler(irq, callback);
```

### `pmm`
```cpp
import pmm;
pmm::init(mb_info);
pmm::alloc_page, pmm::free_page, pmm::alloc_pages;
pmm::total_frames, pmm::free_frames;
```

### `heap`
```cpp
import heap;
heap::init(size), heap::alloc(size), heap::free(ptr);
heap::calloc(num, size), heap::realloc(ptr, new_size);
```

### `terminal`
```cpp
import terminal;
terminal::init();
terminal::send_key(scancode);
```

### `utils.string`
```cpp
import utils.string;
utils::string::strcmp, utils::string::sprintf, utils::string::vsprintf;
```

### `irq.pit` / `irq.kbd`
```cpp
import irq.pit;
irq::pit::pit_init(); irq::pit::timer_handler();

import irq.kbd;
irq::kbd::kbd_init(); irq::kbd::keyboard_handler();
```

## License

MIT
