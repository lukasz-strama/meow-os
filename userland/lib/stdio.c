#include "stdio.h"
#include "syscalls.h"
#include <stdarg.h>

int stdin = 0;
int stdout = 1;

void __libc_init() {
    // Open default devices.
    // Kernel allocates FDs sequentially starting from 0.
    // We assume 0 and 1 are free at startup.
    int fd0 = fopen("/dev/keyboard", "r"); // Should be 0
    int fd1 = fopen("/dev/console", "w");  // Should be 1
    
    if (fd0 != 0 || fd1 != 1) {
        // Something went wrong, maybe FDs were already taken?
        // For now, just assign them.
        stdin = fd0;
        stdout = fd1;
    } else {
        stdin = 0;
        stdout = 1;
    }
}

void putchar(char c) {
    // Write 1 byte to stdout (FD 1)
    sys_write(stdout, &c, 1);
}

// Helper to print a raw string using putchar
void puts(const char* str) {
    while(*str) {
        putchar(*str++);
    }
}

// Helper for integers
void print_dec(int num) {
    char buf[32];
    int i = 0;
    if (num == 0) { putchar('0'); return; }
    if (num < 0) { putchar('-'); num = -num; }

    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (--i >= 0) putchar(buf[i]);
}

// Helper for hex
void print_hex(unsigned int num) {
    char buf[32];
    int i = 0;
    if (num == 0) { putchar('0'); return; }

    while (num > 0) {
        int digit = num % 16;
        buf[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
        num /= 16;
    }
    while (--i >= 0) putchar(buf[i]);
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (const char* p = format; *p != '\0'; p++) {
        if (*p != '%') {
            putchar(*p);
            continue;
        }
        p++; // Skip '%'
        switch (*p) {
            case 's': puts(va_arg(args, char*)); break;
            case 'c': putchar(va_arg(args, int)); break;
            case 'd': print_dec(va_arg(args, int)); break;
            case 'x': print_hex(va_arg(args, unsigned int)); break;
            case '%': putchar('%'); break;
            default:  putchar('%'); putchar(*p); break;
        }
    }

    va_end(args);
    return 0;
}

char* gets(char* buffer, int max_len) {
    int i = 0;
    while (1) {
        char c = sys_getch();
        
        if (c == '\n') {
            putchar('\n');
            buffer[i] = '\0';
            return buffer;
        } else if (c == '\b') {
            if (i > 0) {
                // Handle backspace visually
                putchar('\b');
                putchar(' ');
                putchar('\b');
                i--;
            }
        } else {
            if (i < max_len - 1) {
                putchar(c);
                buffer[i] = c;
                i++;
            }
        }
    }
}

char* get_password(char* buffer, int max_len) {
    int i = 0;
    while (1) {
        char c = sys_getch();
        
        if (c == '\n') {
            putchar('\n');
            buffer[i] = '\0';
            return buffer;
        } else if (c == '\b') {
            if (i > 0) {
                putchar('\b');
                putchar(' ');
                putchar('\b');
                i--;
            }
        } else {
            if (i < max_len - 1) {
                putchar('*');
                buffer[i] = c;
                i++;
            }
        }
    }
}
