#include "stdio.h"
#include "syscalls.h"
#include <stdarg.h>

int stdin = 0;
int stdout = 1;

void __libc_init() {
    // Open default devices.
    // Kernel allocates FDs sequentially starting from 0.
    // We assume 0 and 1 are free at startup.
    
    // Try to open keyboard. If we get 0, great. If not, 0 was taken.
    int fd = fopen("/dev/keyboard", "r");
    if (fd == 0) {
        stdin = 0;
    } else {
        // 0 was taken. fd is something else (e.g. 1, 2...).
        // We don't want keyboard on fd > 0 usually, unless we want to read from it?
        // But stdin is 0.
        // If 0 is taken, it means we inherited stdin.
        // So we should close this new fd.
        if (fd >= 0) sys_close(fd);
        stdin = 0;
    }

    // Try to open console. If we get 1, great. If not, 1 was taken.
    fd = fopen("/dev/console", "w");
    if (fd == 1) {
        stdout = 1;
    } else {
        // 1 was taken. fd is something else (e.g. 0, 2...).
        // If 1 is taken, we inherited stdout.
        // Close this new fd.
        if (fd >= 0) sys_close(fd);
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
