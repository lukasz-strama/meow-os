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

    cli                 ; 1. Disable Interrupts (Critical for stability test)

    ; 2. Visual Debug: Write a big red 'R' (Ring 3) at 0xB8000
    mov rax, 0xB8000
    mov byte [rax], 'R'
    mov byte [rax+1], 0x4F ; Red background, White text

    ; 3. Construct IRETQ Stack Frame
    ; Stack grows downwards. We push: SS, RSP, RFLAGS, CS, RIP

    ; SS (User Data Selector)
    ; Index 3 in GDT_FIX -> 0x18. RPL 3 -> 0x1B.
    push 0x1B

    ; RSP (User Stack)
    push rsi

    ; RFLAGS
    ; 0x202 = Interrupts Enabled (IF=1, Reserved=1)
    ; 0x002 = Interrupts Disabled (IF=0, Reserved=1)
    ; LET'S USE 0x002 TO PREVENT IRQ CRASHES FOR NOW
    push 0x002

    ; CS (User Code Selector)
    ; Index 4 in GDT_FIX -> 0x20. RPL 3 -> 0x23.
    push 0x23

    ; RIP (Entry Point)
    push rdi

    ; 4. Jump!
    iretq
