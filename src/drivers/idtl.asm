; idt.asm
BITS 64

global isr0
global load_idt
extern isr_handler

%macro ISR_COMMON 0
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

    cld

    mov rdi, rsp

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

    add rsp, 16        ; remove int_no + err_code
    iretq
%endmacro

%macro ISR_NOERR 1
global isr%1
isr%1:
    push 0            ; fake error code
    push %1           ; interrupt number
    ISR_COMMON
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push %1           ; interrupt number (error code already on stack)
    ISR_COMMON
%endmacro

; no error code
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7

; has error code
ISR_ERR   8

ISR_NOERR 9

ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14

ISR_NOERR 15

ISR_NOERR 16
ISR_ERR   17

ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; -------------------------
; load IDT
; -------------------------
load_idt:
    lidt [rdi]
    ret

; -------------------------
; test_iretq_asm: pure asm iretq test
; -------------------------
global test_iretq_asm
test_iretq_asm:
    ; QEMU's iretq in 64-bit mode always pops SS and RSP from stack
    ; (even for same-CPL returns), because do_interrupt64 always pushes them.
    ; So we need a 5-value frame: SS, RSP, RFLAGS, CS, RIP
    ;
    ; Entry: RSP = E0 (points to caller's return addr)
    push qword 0x10      ; SS=0x10 at [E0-8]
    lea rax, [rsp + 8]   ; rax = (E0-8)+8 = E0 (entry RSP)
    push rax             ; NEW_RSP=E0 at [E0-16]
    push qword 0x02      ; RFLAGS at [E0-24]
    push qword 0x08      ; CS at [E0-32]
    lea rax, [rel .ret]
    push rax             ; RIP at [E0-40]
    iretq
    ; After iretq: RSP=E0, RIP=.ret, CS=0x08, RFL=0x02, SS=0x10
.ret:
    ret                  ; pops caller ret addr from [E0], RSP=E0+8, returns
