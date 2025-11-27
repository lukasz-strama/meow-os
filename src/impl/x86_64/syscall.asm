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
    push rcx
    push r11
    
    ; Save callee-saved registers
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; 4. Reload Data Segments (Safety)
    mov ax, 0x10 ; Kernel Data
    mov ds, ax
    mov es, ax
    ; fs and gs are usually 0 or special, let's zero them for now
    xor ax, ax
    mov fs, ax
    mov gs, ax

    ; 5. Setup C Arguments
    ; User: RAX=ID, RDI=Arg1
    ; SysV ABI: RDI=Arg1, RSI=Arg2
    mov rdx, rdi    ; Save User Arg1 (pointer) temp
    mov rdi, rax    ; Pass ID as 1st Arg to C
    mov rsi, rdx    ; Pass Ptr as 2nd Arg to C

    call syscall_handler_c

    ; 6. Restore State
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    
    pop r11 ; RFLAGS
    pop rcx ; RIP

    ; 7. Construct IRETQ Frame (Stack grows down)
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
