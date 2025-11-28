#include "syscalls.h"

long syscall1(long num, long arg1) {
    long ret;
    asm volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(arg1)
        : "rcx", "r11", "memory"
    );
    return ret;
}

void sys_print(char* msg) {
    syscall1(0, (long)msg);
}

void sys_putc(char c) {
    syscall1(2, (long)c);
}

void sys_exit(int code) {
    syscall1(1, (long)code);
    while(1); // Should not return
}

unsigned long sys_get_ticks() {
    return (unsigned long)syscall1(3, 0);
}

int sys_kbhit() {
    return (int)syscall1(4, 0);
}

char sys_getch() {
    return (char)syscall1(5, 0);
}

void sys_clear() {
    syscall1(6, 0);
}

void sys_gotoxy(int x, int y) {
    unsigned long packed = ((unsigned long)x << 32) | (unsigned long)y;
    syscall1(7, packed);
}

void sys_set_color(unsigned char fg, unsigned char bg) {
    long packed = (fg << 8) | bg;
    syscall1(8, packed);
}
