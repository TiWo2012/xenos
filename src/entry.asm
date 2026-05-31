BITS 32

section .multiboot
align 8

    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd -(0xe85250d6 + 0 + (header_end - header_start))

header_start:
    dw 0
    dw 0
    dd 8
header_end:

; =========================
; 32-bit data
; =========================

section .bss
align 4096

pml4: resb 4096
pdpt: resb 4096
pd:   resb 4096

stack: resb 16384
stack_top:

mb_magic: resd 1
mb_info:  resd 1

; =========================
; 32-bit entry
; =========================

section .text
global _start
extern boot_main

_start:
    cli

    ; save multiboot
    mov [mb_magic], eax
    mov [mb_info], ebx

    ; set stack (32-bit!)
    mov esp, stack_top

    ; --------------------------------
    ; zero paging structures (32-bit safe)
    ; --------------------------------
    mov edi, pml4
    mov ecx, (4096 * 3) / 4
    xor eax, eax
    rep stosd

    ; --------------------------------
    ; paging setup (identity map 1GB)
    ; --------------------------------

    ; PML4 -> PDPT
    mov eax, pdpt
    or eax, 0b11
    mov [pml4], eax

    ; PDPT -> PD
    mov eax, pd
    or eax, 0b11
    mov [pdpt], eax

    ; fill PD with 2MB pages
    mov ecx, 512
    xor ebx, ebx

.map_pd:
    mov eax, ebx
    shl eax, 21            ; 2MB chunks
    or eax, 0b10000011
    mov [pd + ebx*8], eax
    inc ebx
    loop .map_pd

    ; --------------------------------
    ; enable PAE
    ; --------------------------------
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    ; load PML4 (CR3 is still 32-bit safe here)
    mov eax, pml4
    mov cr3, eax

    ; --------------------------------
    ; enable long mode
    ; --------------------------------
    mov ecx, 0xC0000080
    rdmsr
    or eax, (1 << 8)
    wrmsr

    ; enable paging
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    ; load GDT
    lgdt [gdt_descriptor]

    ; jump to 64-bit mode
    jmp 0x08:long_mode_entry


; =========================
; 64-bit mode
; =========================

BITS 64

long_mode_entry:
    mov rsp, stack_top

    ; restore args (safe in 64-bit now)
    mov edi, [mb_magic]
    mov esi, [mb_info]

    call boot_main

.hang:
    hlt
    jmp .hang


; =========================
; GDT
; =========================

section .rodata

gdt_start:
    dq 0x0000000000000000
    dq 0x00af9a000000ffff
    dq 0x00af92000000ffff

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
