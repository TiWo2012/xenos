ENTRY(_start)

KERNEL_OFFSET = 0xFFFFFFFF80000000;

SECTIONS
{
    . = 1M;

    .boot : ALIGN(4K)
    {
        *(.boot*)
    }

    .boot.bss : ALIGN(4K)
    {
        *(.boot.bss*)
    }

    . += KERNEL_OFFSET;

    KERNEL_START = .;

    .text : AT(ADDR(.text) - KERNEL_OFFSET) ALIGN(4K) { *(.text*) }

    .rodata : AT(ADDR(.rodata) - KERNEL_OFFSET) ALIGN(4K) { *(.rodata*) }

    .data : AT(ADDR(.data) - KERNEL_OFFSET) ALIGN(4K) { *(.data*) }

    .bss : AT(ADDR(.bss) - KERNEL_OFFSET) ALIGN(4K)
    {
        *(COMMON)
        *(.bss*)
    }

    KERNEL_END = .;
}
