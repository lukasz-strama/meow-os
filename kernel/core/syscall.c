#include "core/syscall.h"
#include "drivers/print.h"
#include "drivers/keyboard.h"
#include "drivers/io.h"
#include "fs/fat.h"
#include "core/gdt.h"
#include "core/loader.h"
#include "core/session.h"

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
            // printf("\nProgram exited with code %d\n", (int)arg1);
            asm volatile("sti"); 
            
            // Try to reload shell
            if (program_load("SHELL.BIN") != 0) {
                printf("PANIC: Failed to reload shell!\n");
                kmonitor_init();
            }
            return 0;
        case 2: // sys_putc
            print_char((char)arg1);
            return 0;
        case 3: // sys_get_ticks
            return timer_ticks;
        case 4: // sys_kbhit
            return keyboard_has_data();
        case 5: // sys_getch
            // CRITICAL: Enable interrupts so Keyboard ISR can fill the buffer!
            asm volatile("sti");
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
        case 8: // sys_set_color
            // Unpack fg and bg from arg1 (fg << 8 | bg)
            uint8_t fg = (uint8_t)(arg1 >> 8);
            uint8_t bg = (uint8_t)(arg1 & 0xFF);
            print_set_color(fg, bg);
            return 0;
        case 9: // sys_exec
        {
            char* filename = (char*)arg1;
            return program_load(filename);
        }
        case 10: // sys_ls
            fat_ls();
            return 0;
        case 11: // sys_read_file
            fat_read_file((char*)arg1);
            return 0;
        case 12: // sys_create_file
        {
            // arg1 is pointer to struct { char* name; char* content; }
            // But we only have 1 arg. Let's assume arg1 is filename, and we need another syscall or pack args.
            // Wait, syscall1 only takes 1 arg. We need syscall2 or pack them.
            // Let's pack them into a struct or array.
            // Or just implement syscall2.
            // For now, let's assume arg1 points to a struct with 2 pointers.
            void** args = (void**)arg1;
            fat_create_file((char*)args[0], (char*)args[1]);
            return 0;
        }
        case 13: // sys_delete_file
            fat_delete_file((char*)arg1);
            return 0;
        case 14: // sys_kmonitor
            asm volatile("sti");
            session_logout(); // Logout when entering KMonitor
            kmonitor_init();
            return 0;
        case 15: // sys_login
            session_login((char*)arg1);
            return 0;
        case 16: // sys_get_user
            session_get_username((char*)arg1);
            return 0;
        case 17: // sys_read_file_content
        {
            void** args = (void**)arg1;
            return fat_read_file_to_buffer((char*)args[0], (char*)args[1], (int)(long)args[2]);
        }
        case 18: // sys_shutdown
            // QEMU Shutdown (0x604, 0x2000)
            outw(0x604, 0x2000);
            // Bochs/Older QEMU (0xB004, 0x2000)
            outw(0xB004, 0x2000);
            printf("It is now safe to turn off your computer.\n");
            asm volatile("cli; hlt");
            return 0;
        case 19: // sys_reboot
            // Keyboard Controller Reboot
            uint8_t good = 0x02;
            while (good & 0x02)
                good = inb(0x64);
            outb(0x64, 0xFE);
            asm volatile("cli; hlt");
            return 0;
    }
    return 0;
}
