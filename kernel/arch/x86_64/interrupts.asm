global idt_load
global isr_stub_table
extern isr_handler_c
extern exception_handler
extern gp_handler
extern pf_handler
extern keyboard_handle
extern timer_handler

section .text
bits 64

idt_load:
	lidt [rdi]
	ret

; Macro for exceptions without error code
%macro ISR_NOERRCODE 1
global isr_%1
isr_%1:
    push 0                  ; Push dummy error code
    push %1                 ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Macro for exceptions with error code
%macro ISR_ERRCODE 1
global isr_%1
isr_%1:
    push %1                 ; Push interrupt number
    jmp isr_common_stub
%endmacro

; Define ISRs
ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE   8
ISR_NOERRCODE 9
ISR_ERRCODE   10
ISR_ERRCODE   11
ISR_ERRCODE   12
ISR_ERRCODE   13
ISR_ERRCODE   14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_ERRCODE   17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_ERRCODE   30
ISR_NOERRCODE 31

; Common Stub
isr_common_stub:
	push r15
	push r14
	push r13
	push r12
	push rbp
	push rbx
	push r11
	push r10
	push r9
	push r8
	push rax
	push rcx
	push rdx
	push rsi
	push rdi

    mov rdi, rsp ; Pass Stack Pointer
	call isr_handler_c

	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	pop r8
	pop r9
	pop r10
	pop r11
	pop rbx
	pop rbp
	pop r12
	pop r13
	pop r14
	pop r15
    
    add rsp, 16 ; Pop Interrupt Number and Error Code
	iretq

; IRQ Handlers (32, 33)
global irq0_handler
global isr_keyboard_stub

irq0_handler:
    push 0 ; Dummy
    push 32 ; Vector
    
    ; Save Context
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

    ; Kernel Segments
    cld
    xor rax, rax
    mov ax, ds
    push rax
    mov ax, 0x10
    mov ds, ax
    mov es, ax

    call timer_handler

    pop rax
    mov ds, ax
    mov es, ax

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
    
    add rsp, 16
    iretq

isr_keyboard_stub:
    push 0
    push 33
    
	push r15
	push r14
	push r13
	push r12
	push rbp
	push rbx
	push r11
	push r10
	push r9
	push r8
	push rax
	push rcx
	push rdx
	push rsi
	push rdi

	call keyboard_handle

	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	pop r8
	pop r9
	pop r10
	pop r11
	pop rbx
	pop rbp
	pop r12
	pop r13
	pop r14
	pop r15

    add rsp, 16
	iretq

; Table of ISR pointers for C to access
section .data
global isr_table
isr_table:
    dq isr_0, isr_1, isr_2, isr_3, isr_4, isr_5, isr_6, isr_7
    dq isr_8, isr_9, isr_10, isr_11, isr_12, isr_13, isr_14, isr_15
    dq isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23
    dq isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31

