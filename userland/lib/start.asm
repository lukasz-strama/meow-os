[BITS 64]
global _start
extern main
extern sys_exit

section .text
_start:
    ; CRITICAL: System V ABI requires 16-byte stack alignment
    ; before a call. We force it here.
    and rsp, -16 

    call main

    ; Exit with code 0 if main returns
    mov rdi, 0
    call sys_exit
    jmp $
