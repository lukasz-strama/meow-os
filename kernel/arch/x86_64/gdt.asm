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
    ; Arg 3 (RDX) = argc (User RDI)
    ; Arg 4 (RCX) = argv (User RSI)

    cli                 ; 1. Disable Interrupts (Critical for stability test)

    ; 2. Construct IRETQ Stack Frame
    ; Stack grows downwards. We push: SS, RSP, RFLAGS, CS, RIP

    ; SS (User Data Selector)
    ; Index 3 in GDT_FIX -> 0x18. RPL 3 -> 0x1B.
    push 0x1B

    ; RSP (User Stack)
    push rsi

    ; RFLAGS
    ; 0x202 = Interrupts Enabled (IF=1, Reserved=1)
    ; 0x002 = Interrupts Disabled (IF=0, Reserved=1)
    ; ENABLE INTERRUPTS so Timer works in User Mode!
    push 0x202

    ; CS (User Code Selector)
    ; Index 4 in GDT_FIX -> 0x20. RPL 3 -> 0x23.
    push 0x23

    ; RIP (Entry Point)
    push rdi

    ; Set User Arguments (System V ABI: RDI, RSI)
    mov rdi, rdx    ; argc
    mov rsi, rcx    ; argv

    ; Clear other registers to avoid leaking kernel info
    xor rax, rax
    xor rbx, rbx
    xor rdx, rdx
    xor rcx, rcx
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    xor rbp, rbp

    ; 4. Jump!
    iretq
