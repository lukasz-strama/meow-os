[BITS 64]
global _start
extern main
extern sys_exit
extern __libc_init

section .text
_start:
    ; Save argc (RDI) and argv (RSI) in callee-saved registers
    mov r12, rdi
    mov r13, rsi

    ; CRITICAL: System V ABI requires 16-byte stack alignment
    ; before a call. We force it here.
    and rsp, -16 

    call __libc_init
    
    ; Restore argc and argv for main
    mov rdi, r12
    mov rsi, r13
    
    call main

    ; Exit with code 0 if main returns
    mov rdi, 0
    call sys_exit
    jmp $
