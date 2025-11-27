#include "core/syscall.h"
#include "drivers/print.h"

#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_SFMASK 0xC0000084

extern void syscall_entry();

// 4KB Stack for Syscalls
uint8_t syscall_stack[4096];
uint64_t syscall_stack_top = (uint64_t)syscall_stack + 4096;

void syscall_init() {
    // STAR: Bits 32-47 = Kernel CS (0x08), Bits 48-63 = User CS Base (0x10)
    // Syscall CS = 0x08, SS = 0x10
    // Sysret CS = 0x20 (0x10+16), SS = 0x18 (0x10+8)
    // Note: We use 0x13 (0x10 | 3) to set RPL to 3 just in case
    uint64_t star = ((uint64_t)0x13 << 48) | ((uint64_t)0x08 << 32);
    wrmsr(MSR_STAR, star);
    
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);
    wrmsr(MSR_SFMASK, 0x200); // Disable interrupts on syscall entry
    
    printf("Syscalls Initialized.\n");
}

void syscall_handler_c(uint64_t syscall_id, uint64_t arg1) {
    if (syscall_id == 0) {
        printf("%s", (char*)arg1);
    }
}
