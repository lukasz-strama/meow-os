global start
extern kernel_main

section .text
bits 32
start:
	cli ; Ensure interrupts are disabled
	mov esp, stack_top
	mov [multiboot_info_ptr], ebx ; Save Multiboot info pointer
    mov [multiboot_magic_ptr], eax ; Save Multiboot magic number

	call check_multiboot
	call check_cpuid
	call check_long_mode

	call setup_page_tables
	call enable_paging

	lgdt [gdt64.pointer]
	jmp gdt64.code_segment:long_mode_start

check_multiboot:
	cmp eax, 0x36d76289
	jne .no_multiboot
	ret
.no_multiboot:
	mov al, "M"
	jmp error

check_cpuid:
	pushfd
	pop eax
	mov ecx, eax
	xor eax, 1 << 21
	push eax
	popfd
	pushfd
	pop eax
	push ecx
	popfd
	cmp eax, ecx
	je .no_cpuid
	ret
.no_cpuid:
	mov al, "C"
	jmp error

check_long_mode:
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb .no_long_mode

	mov eax, 0x80000001
	cpuid
	test edx, 1 << 29
	jz .no_long_mode
	ret
.no_long_mode:
	mov al, "L"
	jmp error

setup_page_tables:
	mov eax, page_table_l3
	or eax, 0x7 ; present, writable, user
	mov [page_table_l4], eax

	mov eax, page_table_l2
	or eax, 0x7 ; present, writable, user
	mov [page_table_l3], eax

	mov eax, 0 ; counter
.map_pd:
	mov ebx, 0x200000
	imul ebx, eax
	or ebx, 0x87 ; present, writable, huge page, user
	mov [page_table_l2 + eax * 8], ebx
	mov dword [page_table_l2 + eax * 8 + 4], 0 ; Ensure high dword is 0

	inc eax
	cmp eax, 512
	jne .map_pd

	ret

enable_paging:
	; pass page table location to cpu
	mov eax, page_table_l4
	mov cr3, eax

	; enable PAE
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	; enable long mode
	mov ecx, 0xC0000080
	rdmsr
	or eax, (1 << 8) | 1 ; Enable LME (Bit 8) and SCE (Bit 0) for Syscalls
	wrmsr

	; enable paging
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	ret

error:
	; print "ERR: X" where X is the error code
	mov dword [0xb8000], 0x4f524f45
	mov dword [0xb8004], 0x4f3a4f52
	mov dword [0xb8008], 0x4f204f20
	mov byte  [0xb800a], al
	hlt

section .bss
align 4096
page_table_l4:
	resb 4096
page_table_l3:
	resb 4096
page_table_l2:
	resb 4096
stack_bottom:
	resb 4096 * 4
stack_top:
multiboot_info_ptr:
    resd 1
multiboot_magic_ptr:
    resd 1

section .rodata
gdt64:
	dq 0 ; zero entry
.code_segment: equ $ - gdt64
	dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; code segment
.pointer:
	dw $ - gdt64 - 1
	dq gdt64

section .text
bits 64
long_mode_start:
	; load null into all data segment registers
	mov ax, 0
	mov ss, ax
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

    ; Restore multiboot pointer to rdi (first argument)
    ; Restore multiboot magic to rsi (second argument) - WAIT, ABI says RDI=1st, RSI=2nd.
    ; The C function is kernel_main(uint64_t magic, uint64_t addr)
    ; So RDI = magic, RSI = addr
    mov edi, [multiboot_magic_ptr]
    mov esi, [multiboot_info_ptr]
    
	call kernel_main
	hlt
