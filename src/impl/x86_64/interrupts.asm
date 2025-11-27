global idt_load
global isr_stub
extern isr_handler_c

section .text
bits 64

idt_load:
	lidt [rdi]
	ret

isr_stub:
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

	iretq
