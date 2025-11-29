#include "core/syscall.h"
#include "drivers/print.h"
#include "drivers/keyboard.h"
#include "drivers/io.h"
#include "fs/fat.h"
#include "core/gdt.h"
#include "core/loader.h"
#include "core/session.h"
#include "core/process.h"
#include "memory/pmm.h"
#include "fs/vfs.h"

#define MSR_STAR 0xC0000081
#define MSR_LSTAR 0xC0000082
#define MSR_SFMASK 0xC0000084

extern void syscall_entry();
extern void kmonitor_init();
extern volatile uint64_t timer_ticks;

// 4KB Stack for Syscalls
uint8_t syscall_stack[4096];
uint64_t syscall_stack_top = (uint64_t)syscall_stack + 4096;

int validate_ptr(void* ptr) {
    if (ptr == 0) return 0;
    // TODO: Add range check (e.g. < 0x8000000000000000)
    return 1;
}

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
            if (!validate_ptr((void*)arg1)) return 1;
            printf("%s", (char*)arg1);
            return 0;
        case 1: // sys_exit
            // printf("\nProgram exited with code %d\n", (int)arg1);
            asm volatile("sti"); 
            
            // Try to reload shell
            if (program_load("/BIN/SHELL.BIN") != 0) {
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
            if (!validate_ptr((void*)arg1)) return 1;
            char* filename = (char*)arg1;
            return program_load(filename);
        }
        case 10: // sys_ls
            if (arg1 == 0) fat_ls("/");
            else {
                if (!validate_ptr((void*)arg1)) return 1;
                fat_ls((char*)arg1);
            }
            return 0;
        case 11: // sys_read_file
            if (!validate_ptr((void*)arg1)) return 1;
            fat_read_file((char*)arg1);
            return 0;
        case 12: // sys_create_file
        {
            if (!validate_ptr((void*)arg1)) return 1;
            void** args = (void**)arg1;
            if (!validate_ptr(args[0]) || !validate_ptr(args[1])) return 1;
            fat_create_file((char*)args[0], (char*)args[1]);
            return 0;
        }
        case 13: // sys_delete_file
            if (!validate_ptr((void*)arg1)) return 1;
            fat_delete_file((char*)arg1);
            return 0;
        case 14: // sys_kmonitor
            asm volatile("sti");
            session_logout(); // Logout when entering KMonitor
            kmonitor_init();
            return 0;
        case 15: // sys_login
            if (!validate_ptr((void*)arg1)) return 1;
            session_login((char*)arg1);
            return 0;
        case 16: // sys_get_user
            if (!validate_ptr((void*)arg1)) return 1;
            session_get_username((char*)arg1);
            return 0;
        case 17: // sys_read_file_content
        {
            if (!validate_ptr((void*)arg1)) return 1;
            void** args = (void**)arg1;
            if (!validate_ptr(args[0]) || !validate_ptr(args[1])) return 1;
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
        case 20: // sys_mkdir
            if (!validate_ptr((void*)arg1)) return 1;
            fat_mkdir((char*)arg1);
            return 0;
        case 21: // sys_stat
            {
                void** args = (void**)arg1;
                char* path = (char*)args[0];
                unsigned int* size = (unsigned int*)args[1];
                int* is_dir = (int*)args[2];
                
                uint16_t cluster;
                uint32_t fsize;
                uint8_t fis_dir;
                
                if (fat_resolve_path(path, &cluster, &fsize, &fis_dir)) {
                    if (size) *size = fsize;
                    if (is_dir) *is_dir = fis_dir;
                    return 0; // Success
                } else {
                    return -1; // Not found
                }
            }
            break;
        case 22: // sys_rmdir
            fat_rmdir((char*)arg1);
            break;
        case 23: // sys_get_proc_info
        {
            if (!validate_ptr((void*)arg1)) return 1; // arg1 is pid? No, arg1 is usually a pointer or value.
            // syscall1(num, arg1).
            // We need 2 args: pid and buffer.
            // syscall1 only passes 1 arg.
            // We need to pass a struct or array of args if we have more than 1.
            // Or use syscall2 if we had it.
            // The current syscall implementation only supports 1 argument `syscall1`.
            // So we must pass args as a pointer to array.
            void** args = (void**)arg1;
            if (!validate_ptr(args)) return 1;
            int pid = (int)(long)args[0];
            ProcessInfo* info = (ProcessInfo*)args[1];
            if (!validate_ptr(info)) return 1;
            return get_process_info(pid, info);
        }
        case 24: // sys_get_mem_info
        {
            if (!validate_ptr((void*)arg1)) return 1;
            MemInfo* info = (MemInfo*)arg1;
            pmm_get_info(info);
            return 0;
        }
        case 25: // sys_open
        {
            if (!validate_ptr((void*)arg1)) return -1;
            void** args = (void**)arg1;
            char* filename = (char*)args[0];
            int flags = (int)(long)args[1];
            return vfs_open_file(filename, flags);
        }
        case 26: // sys_close
        {
            int fd = (int)arg1;
            vfs_close_file(fd);
            return 0;
        }
        case 27: // sys_read
        {
            // Enable interrupts to allow keyboard input
            asm volatile("sti");
            if (!validate_ptr((void*)arg1)) return -1;
            void** args = (void**)arg1;
            int fd = (int)(long)args[0];
            uint8_t* buffer = (uint8_t*)args[1];
            uint32_t size = (uint32_t)(long)args[2];
            return vfs_read_file(fd, buffer, size);
        }
        case 28: // sys_write
        {
            if (!validate_ptr((void*)arg1)) return -1;
            void** args = (void**)arg1;
            int fd = (int)(long)args[0];
            uint8_t* buffer = (uint8_t*)args[1];
            uint32_t size = (uint32_t)(long)args[2];
            return vfs_write_file(fd, buffer, size);
        }
        case 29: // sys_read_dir
        {
            if (!validate_ptr((void*)arg1)) return -1;
            void** args = (void**)arg1;
            char* path = (char*)args[0];
            int index = (int)(long)args[1];
            char* out_name = (char*)args[2];
            uint32_t* out_size = (uint32_t*)args[3];
            int* out_is_dir = (int*)args[4];
            
            if (!validate_ptr(path) || !validate_ptr(out_name)) return -1;
            
            return fat_read_dir_entry(path, index, out_name, out_size, out_is_dir);
        }
        default:
            break;
    }
    return 0;
}
