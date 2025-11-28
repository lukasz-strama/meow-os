#ifndef SYSCALLS_H
#define SYSCALLS_H

void sys_print(char* msg);
void sys_putc(char c);
long syscall1(long number, long arg1);
void sys_exit(int code);
unsigned long sys_get_ticks();
int sys_kbhit();
char sys_getch();
void sys_clear();
void sys_gotoxy(int x, int y);

#endif
