#ifndef SYSCALLS_H
#define SYSCALLS_H

void sys_print(char* msg);
void sys_putc(char c);
long syscall1(long number, long arg1);
void sys_exit(int code);

#endif
