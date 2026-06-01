; idt.asm
BITS 64

global isr0
global load_idt
extern isr_handler

; -------------------------
; ISR 0 (divide by zero)
; -------------------------
isr0:
    push 0              ; error code (fake)
    push 0              ; interrupt number

    ; save registers
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp        ; argument = pointer to stack
    call isr_handler

    ; restore
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16         ; remove int_no + error_code
    iretq

; -------------------------
; load IDT
; -------------------------
load_idt:
    lidt [rdi]
    ret
