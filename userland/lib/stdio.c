#include "stdio.h"
#include "syscalls.h"
#include <stdarg.h>

// Helper to print a raw string using putc
void puts(const char* str) {
    while(*str) {
        sys_putc(*str++);
    }
}

// Helper for integers
void print_dec(int num) {
    char buf[32];
    int i = 0;
    if (num == 0) { sys_putc('0'); return; }
    if (num < 0) { sys_putc('-'); num = -num; }

    while (num > 0) {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }
    while (--i >= 0) sys_putc(buf[i]);
}

// Helper for hex
void print_hex(unsigned int num) {
    char buf[32];
    int i = 0;
    if (num == 0) { sys_putc('0'); return; }

    while (num > 0) {
        int digit = num % 16;
        buf[i++] = (digit < 10) ? (digit + '0') : (digit - 10 + 'a');
        num /= 16;
    }
    while (--i >= 0) sys_putc(buf[i]);
}

int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (const char* p = format; *p != '\0'; p++) {
        if (*p != '%') {
            sys_putc(*p);
            continue;
        }
        p++; // Skip '%'
        switch (*p) {
            case 's': puts(va_arg(args, char*)); break;
            case 'c': sys_putc(va_arg(args, int)); break;
            case 'd': print_dec(va_arg(args, int)); break;
            case 'x': print_hex(va_arg(args, unsigned int)); break;
            case '%': sys_putc('%'); break;
            default:  sys_putc('%'); sys_putc(*p); break;
        }
    }

    va_end(args);
    return 0;
}
