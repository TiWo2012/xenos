global asm_shutdown

asm_shutdown:
    mov dx, 0x604       ; ACPI PM1a_CNT port (example address)
    mov ax, 0x2000      ; SLP_TYP | SLP_EN (exact bytes depend on motherboard)
    out dx, ax          ; Send command to the power management port

