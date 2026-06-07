BITS 32

; multiboot header must be in first 8KB — placed at start of .boot section
section .boot alloc exec write
align 8

    dd 0xe85250d6
    dd 0
    dd header_end - header_start
    dd -(0xe85250d6 + 0 + (header_end - header_start))

header_start:
    align 8
    dw 5
    dw 0
    dd 20
    dd 1024
    dd 768
    dd 32

    align 8
    dw 0
    dw 0
    dd 8
header_end:

; =========================
; Boot data (low mem, identity mapped)
; =========================

section .boot.bss nobits alloc write
align 4096

pml4:       resb 4096
pdpt:       resb 4096
pd:         resb 4096
pdpt_phys:  resb 4096
pd_phys:    resb 16384
stack:      resb 16384
stack_top:
mb_magic:   resd 1
mb_info:    resd 1

; =========================
; Boot code (low mem, identity mapped)
; =========================

section .boot alloc exec write
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
    mov ecx, (4096 * 8) / 4
    xor eax, eax
    rep stosd

    ; PML4[0] -> PDPT (identity map, will be cleared after boot)
    mov eax, pdpt
    or eax, 0b11
    mov [pml4], eax

    ; PML4[256] -> PDPT_phys (physmap: physical 0-4GB at 0xFFFF800000000000+)
    mov eax, pdpt_phys
    or eax, 0b11
    mov [pml4 + 256*8], eax

    ; PML4[510] -> PML4 itself (recursive page table mapping)
    mov eax, pml4
    or eax, 0b11
    mov [pml4 + 510*8], eax

    ; PML4[511] -> same PDPT (higher-half kernel alias)
    mov eax, pdpt
    or eax, 0b11
    mov [pml4 + 511*8], eax

    ; PDPT[0] -> PD (identity map / PML4[0])
    mov eax, pd
    or eax, 0b11
    mov [pdpt], eax

    ; PDPT[510] -> same PD (higher half / PML4[511])
    mov [pdpt + 510*8], eax

    ; fill PD with 2MB pages (covers 0-1GB)
    mov ecx, 512
    xor ebx, ebx
.map_pd:
    mov eax, ebx
    shl eax, 21
    or eax, 0b10000011
    mov [pd + ebx*8], eax
    inc ebx
    loop .map_pd

    ; PDPT_phys[0..3] -> PD_phys + N*4096 (physmap, 4GB range)
    mov eax, pd_phys
    or eax, 0b11
    mov [pdpt_phys], eax

    mov eax, pd_phys + 4096
    or eax, 0b11
    mov [pdpt_phys + 1*8], eax

    mov eax, pd_phys + 8192
    or eax, 0b11
    mov [pdpt_phys + 2*8], eax

    mov eax, pd_phys + 12288
    or eax, 0b11
    mov [pdpt_phys + 3*8], eax

    ; fill all 4 physmap PDs (2048 entries) with 2MB pages covering 0-4GB
    mov ecx, 2048
    xor ebx, ebx
.map_pd_phys:
    mov eax, ebx
    shl eax, 21
    or eax, 0b10000011
    mov [pd_phys + ebx*8], eax
    inc ebx
    loop .map_pd_phys

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

    ; load GDT (descriptor still at low VMA)
    lgdt [gdt_descriptor]

    ; jump to 64-bit mode (target at low VMA, identity mapped)
    jmp 0x08:long_mode_entry


; =========================
; 64-bit mode entry (low VMA, identity mapped)
; =========================

BITS 64

PHYS_MAP_BASE equ 0xFFFF800000000000

long_mode_entry:
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax

    ; switch stack to physmap (identity map will be cleared later)
    mov rax, PHYS_MAP_BASE + stack_top
    mov rsp, rax

    ; restore args via physmap
    mov rax, PHYS_MAP_BASE + mb_magic
    mov edi, [rax]
    mov rax, PHYS_MAP_BASE + mb_info
    mov esi, [rax]

    ; jump to higher half kernel
    mov rax, boot_main
    call rax

.hang:
    hlt
    jmp .hang


; =========================
; GDT (low VMA, referenced from 32-bit code before far jump)
; =========================

gdt_start:
    dq 0x0000000000000000     ; 0x00 - null
    dq 0x00af9a000000ffff     ; 0x08 - kernel code
    dq 0x00af92000000ffff     ; 0x10 - kernel data
    dq 0x00af92000000ffff     ; 0x18 - kernel data (for bootloader SS match)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
