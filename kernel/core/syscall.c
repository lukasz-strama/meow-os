#include "core/syscall.h"
#include "drivers/print.h"
#include "drivers/keyboard.h"

#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_SFMASK 0xC0000084

extern void syscall_entry();
extern void kmonitor_init();
extern volatile uint64_t timer_ticks;

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

uint64_t syscall_handler_c(uint64_t syscall_id, uint64_t arg1) {
    switch (syscall_id) {
        case 0: // sys_print
            printf("%s", (char*)arg1);
            return 0;
        case 1: // sys_exit
            printf("\nProgram exited with code %d\n", (int)arg1);
            asm volatile("sti"); // Enable interrupts for KMonitor
            kmonitor_init();
            return 0;
        case 2: // sys_putc
            print_char((char)arg1);
            return 0;
        case 3: // sys_get_ticks
            return timer_ticks;
        case 4: // sys_kbhit
            return keyboard_has_data();
        case 5: // sys_getch
            return keyboard_get_char();
        case 6: // sys_clear
            print_clear();
            return 0;
        case 7: // sys_gotoxy
            // Unpack x and y from arg1 (x << 32 | y)
            int x = (int)(arg1 >> 32);
            int y = (int)(arg1 & 0xFFFFFFFF);
            print_set_cursor_position(x, y);
            return 0;
    }
    return 0;
}
