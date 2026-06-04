# Xenos

A hobby x86-64 operating system kernel written from scratch in C++23 and NASM assembly. Boots via GRUB (Multiboot2), runs in long mode with 4-level paging, and provides an interactive shell over VGA text mode.

## Features

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
- nasm
- ld.lld
- grub-mkrescue
- qemu-system-x86_64

## Build & Run

```sh
make          # build kernel ISO
make run      # build and boot in QEMU (serial console on stdio)
make clean    # remove build artifacts
make tags     # generate ctags
```

The `run` target boots the kernel in QEMU with the serial port connected to stdio, so all serial debug output appears in your terminal. The VGA console is also visible in the QEMU window.

## Project Structure

```
├── Makefile                   # Build system (clang++, nasm, ld.lld)
├── linker.ld                  # Linker script (kernel at 1M physical)
├── grub.cfg                   # GRUB boot menu config source
├── test.py                    # Generate ISR extern declarations
├── src/
│   ├── entry.asm              # Multiboot2 header, long-mode init, paging, GDT
│   ├── boot.cpp               # C++ wrapper: calls kernel_main
│   ├── kernel.cpp             # Kernel init: PIC, IDT, PIT, KBD, PMM, heap, terminal
│   └── drivers/
│       ├── binio.h/.cpp       # Port I/O (inb/outb)
│       ├── vga.h/.cpp         # VGA text-mode driver (80×25, hardware cursor, printf)
│       ├── serial.h/.cpp      # Serial COM1 driver (38400 8N1, loopback test, printf)
│       ├── idt.h/.cpp         # IDT setup, PIC management, IRQ dispatch
│       ├── idtl.asm           # Assembly ISR stubs (32 hand-written), load_idt, iretq test
│       ├── isr.cpp            # C++ exception handler with full register dump
│       ├── irq/
│       │   ├── pit.h/.cpp     # PIT timer (1000 Hz)
│       │   └── kbd.h/.cpp     # PS/2 keyboard (scancode set 1, US ANSI)
│       ├── terminal/
│       │   ├── terminal.h/.cpp    # Interactive shell (clear, exit, mem)
│       │   └── asm_shutdown.asm   # ACPI power-off
│       └── mem/
│           ├── pmm.h/.cpp     # Physical memory manager (bitmap, 4 GB max)
│           └── heap.h/.cpp    # Kernel heap (linked-list, splitting, coalescing)
└── utils/
    ├── memory.h/.cpp          # memset (freestanding)
    └── string.h/.cpp          # strcmp, vsprintf, sprintf (freestanding)
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

The Makefile uses `find` for automatic source discovery and `-MMD -MP` for dependency tracking.

- `make compile_commands.json` — generates a `compile_commands.json` for clangd LSP (requires `bear`)
- `os.iso` is produced by `grub-mkrescue`
- QEMU is invoked with `-cdrom os.iso -serial stdio`

## Kernel API

### I/O (`drivers/binio.h`)
- `outb(port, val)` / `inb(port)` — byte port I/O

### VGA (`drivers/vga.h`)
- `vga::write_char`, `vga::write_string`, `vga::printf` — text output
- `vga::backspace`, `vga::clear_scr`, `vga::set_cursor_visible` — cursor/screen control

### Serial (`drivers/serial.h`)
- `serial::printf`, `serial::write_char`, `serial::write_string` — serial output
- `serial::print_hex`, `serial::write_dec` — numeric output

### IDT (`drivers/idt.h`)
- `idt::init`, `idt::pic_remap`, `idt::pic_eoi`
- `idt::irq_register_handler(irq, callback)`

### PMM (`drivers/mem/pmm.h`)
- `pmm::init(mb_info)` — parse multiboot info and init bitmap
- `pmm::alloc_page`, `pmm::free_page`, `pmm::alloc_pages`
- `pmm::total_frames`, `pmm::free_frames`

### Heap (`drivers/mem/heap.h`)
- `heap::init(size)`, `heap::alloc(size)`, `heap::free(ptr)`
- `heap::calloc(num, size)`, `heap::realloc(ptr, new_size)`

### Terminal (`drivers/terminal/terminal.h`)
- `terminal::init()` — start the shell
- `terminal::send_key(scancode)` — feed a scancode (called by the keyboard IRQ handler)

### String utils (`utils/string.h`)
- `utils::string::strcmp`, `utils::string::sprintf`, `utils::string::vsprintf`

## License

MIT
