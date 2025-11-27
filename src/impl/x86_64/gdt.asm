global load_gdt
global load_tss
global enter_user_mode

section .text
bits 64

load_gdt:
    lgdt [rdi]      ; Load GDT from pointer passed in RDI
    
    ; Reload segment registers
    mov ax, 0x10    ; Kernel Data Segment (Offset 0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to reload CS
    push 0x08       ; Kernel Code Segment
    lea rax, [rel .reload_cs]
    push rax
    retfq

.reload_cs:
    ret

load_tss:
    mov ax, di      ; TSS selector passed in RDI
    ltr ax
    ret

enter_user_mode:
    ; Arg 1 (RDI) = Entry Point (RIP)
    ; Arg 2 (RSI) = User Stack (RSP)

    cli             ; Disable interrupts strictly during the switch

    ; Push SS (User Data Selector | 3)
    push 0x1B       ; 0x18 | 3

    ; Push RSP
    push rsi

    ; Push RFLAGS
    pushfq
    pop rax
    ; or rax, 0x200   ; Enable Interrupts (IF) - DISABLED FOR DEBUGGING
    push rax

    ; Push CS (User Code Selector | 3)
    push 0x23       ; 0x20 | 3

    ; Push RIP
    push rdi

    iretq           ; The Jump!
