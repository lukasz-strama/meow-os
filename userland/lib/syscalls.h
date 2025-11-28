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

enum {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GRAY = 7,
    COLOR_DARK_GRAY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_PINK = 13,
    COLOR_YELLOW = 14,
    COLOR_WHITE = 15,
};

void sys_set_color(unsigned char fg, unsigned char bg);
int sys_exec(char* filename);
void sys_ls(char* path);
void sys_cat(char* filename);
void sys_mkfile(char* filename, char* content);
void sys_rm(char* filename);
void sys_kmonitor();
void sys_login(char* username);
void sys_get_user(char* buffer);
int sys_read_file_content(char* filename, char* buffer, int max_len);
void sys_shutdown();
void sys_reboot();
void sys_mkdir(char* path);
int sys_stat(char* path, unsigned int* size, int* is_dir);
void sys_rmdir(char* path);

#endif
