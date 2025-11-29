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

int sys_exec(char* filename) {
    return (int)syscall1(9, (long)filename);
}

void sys_ls(char* path) {
    syscall1(10, (long)path);
}

void sys_cat(char* filename) {
    syscall1(11, (long)filename);
}

void sys_mkfile(char* filename, char* content) {
    void* args[2] = { filename, content };
    syscall1(12, (long)args);
}

void sys_rm(char* filename) {
    syscall1(13, (long)filename);
}

void sys_kmonitor() {
    syscall1(14, 0);
}

void sys_login(char* username) {
    syscall1(15, (long)username);
}

void sys_get_user(char* buffer) {
    syscall1(16, (long)buffer);
}

int sys_read_file_content(char* filename, char* buffer, int max_len) {
    void* args[3] = { filename, buffer, (void*)(long)max_len };
    return (int)syscall1(17, (long)args);
}

void sys_shutdown() {
    syscall1(18, 0);
}

void sys_reboot() {
    syscall1(19, 0);
}

void sys_mkdir(char* path) {
    syscall1(20, (long)path);
}

int sys_stat(char* path, unsigned int* size, int* is_dir) {
    void* args[3] = { path, size, is_dir };
    return (int)syscall1(21, (long)args);
}

void sys_rmdir(char* path) {
    syscall1(22, (long)path);
}

int sys_get_proc_info(int pid, ProcessInfo* info) {
    void* args[2] = { (void*)(long)pid, (void*)info };
    return (int)(long)syscall1(23, (long)args);
}

void sys_get_mem_info(MemInfo* info) {
    syscall1(24, (long)info);
}

int sys_open(char* filename, int flags) {
    void* args[2] = { filename, (void*)(long)flags };
    return (int)syscall1(25, (long)args);
}

void sys_close(int fd) {
    syscall1(26, (long)fd);
}

int sys_read(int fd, void* buffer, int size) {
    void* args[3] = { (void*)(long)fd, buffer, (void*)(long)size };
    return (int)syscall1(27, (long)args);
}

int sys_write(int fd, void* buffer, int size) {
    void* args[3] = { (void*)(long)fd, buffer, (void*)(long)size };
    return (int)syscall1(28, (long)args);
}
