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

void sys_exit() {
    // For now, just loop or crash, as we don't have proper exit handling yet
    // But let's define syscall 1 as exit if we had it.
    // For now, we can just loop.
    while(1);
}
