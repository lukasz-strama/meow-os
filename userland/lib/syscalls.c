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
