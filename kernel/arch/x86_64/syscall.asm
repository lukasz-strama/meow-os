global wrmsr
global syscall_entry
extern syscall_handler_c
extern syscall_stack_top

section .text
bits 64

wrmsr:
    ; RDI = MSR, RSI = Value
    mov ecx, edi    ; MSR index goes in ECX
    mov rax, rsi    ; Copy full 64-bit value to RAX
    mov rdx, rsi    ; Copy full 64-bit value to RDX
    shr rdx, 32     ; Shift high 32 bits down to low 32 bits of RDX
    wrmsr
    ret

syscall_entry:
    ; 1. Swap Stack
    mov [user_rsp_scratch], rsp
    mov rsp, [syscall_stack_top]

    ; 2. Save State (RCX=RIP, R11=RFLAGS from syscall)
    ; Save registers (Do NOT push RAX, we want to return a new value)
    push rcx
    push r11
    push rdi
    push rsi
    push rdx
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; 3. Setup C Arguments
    ; User: RAX=ID, RDI=Arg1
    ; SysV ABI: RDI=Arg1, RSI=Arg2
    mov rsi, rdi    ; Arg1 (from RDI) -> RSI (2nd arg)
    mov rdi, rax    ; ID (from RAX) -> RDI (1st arg)

    ; 4. Reload Data Segments (Safety)
    mov ax, 0x10 ; Kernel Data
    mov ds, ax
    mov es, ax
    xor ax, ax
    mov fs, ax
    mov gs, ax

    call syscall_handler_c

    ; RAX now holds the return value

    ; 5. Restore State
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop rdx
    pop rsi
    pop rdi
    pop r11
    pop rcx

    ; 6. Construct IRETQ Frame (Stack grows down)
    ; Order: SS, RSP, RFLAGS, CS, RIP
    push 0x1B              ; User Data Selector
    push qword [user_rsp_scratch] ; User RSP
    push r11               ; User RFLAGS
    push 0x23              ; User Code Selector
    push rcx               ; User RIP

    ; 7. Return
    iretq

section .bss
user_rsp_scratch: resq 1
