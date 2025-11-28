#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

int printf(const char* format, ...);
char* gets(char* buffer, int max_len);
char* get_password(char* buffer, int max_len);

#endif
