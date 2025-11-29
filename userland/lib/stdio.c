#include "stdio.h"
#include "syscalls.h"
#include <stdarg.h>

int stdin = 0;
int stdout = 1;

void __libc_init() {
    // Initialize standard file descriptors.
    // Attempt to open default devices for stdin (0) and stdout (1).
    
    // Initialize stdin
    int fd = fopen("/dev/keyboard", "r");
    if (fd == 0) {
        stdin = 0;
    } else {
        // FD 0 is already occupied (inherited).
        // Close the newly opened FD as we use the inherited one.
        if (fd >= 0) sys_close(fd);
        stdin = 0;
    }

    // Initialize stdout
    fd = fopen("/dev/console", "w");
    if (fd == 1) {
        stdout = 1;
    } else {
        // FD 1 is already occupied (inherited).
        // Close the newly opened FD as we use the inherited one.
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
