# Xenos

A hobby x86-64 operating system kernel written from scratch.

## Features

- Multiboot2-compliant, boots via GRUB
- 64-bit long mode with 4-level paging (2MB huge pages)
- IDT with handlers for CPU exceptions and IRQs
- PIC remapping and interrupt management
- PIT timer at 1000 Hz
- PS/2 keyboard driver with US ANSI scancode mapping
- VGA text-mode driver (80x25, 0xB8000)
- Serial COM1 driver (38400 8N1)
- Interactive terminal shell (`clear`, `exit` → ACPI shutdown)
- All register state dumped on CPU exception

## Prerequisites

- clang++ (C++23)
- nasm
- ld.lld
- grub-mkrescue
- qemu-system-x86_64

## Build & Run

```sh
make          # build kernel ISO
make run      # build and boot in QEMU (serial console)
make clean    # remove build artifacts
```

## Project Structure

```
src/
├── entry.asm               # Multiboot2 header, long mode init, paging
├── boot.cpp                # C++ boot wrapper
├── kernel.cpp              # Kernel main init
├── drivers/                # Hardware drivers
│   ├── binio               # Port I/O (inb/outb)
│   ├── vga                 # VGA text mode
│   ├── serial              # Serial COM1
│   ├── idt / idtl          # IDT setup + assembly ISR stubs
│   ├── isr                 # C++ interrupt handler
│   ├── irq/pit             # Programmable Interval Timer
│   ├── irq/kbd             # PS/2 keyboard
│   └── terminal            # Shell + ACPI shutdown
└── utils/                  # Freestanding utils (memset, strcmp)
```

## License

MIT
