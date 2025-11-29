#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

int printf(const char* format, ...);
void sprintf(char* buffer, const char* format, ...);

// File I/O
int fopen(char* filename, char* mode);
int fread(void* ptr, int size, int count, int fd); // Note: Standard fread is (ptr, size, count, stream). I'll adapt.
int fwrite(void* ptr, int size, int count, int fd);
void fclose(int fd);

char* gets(char* buffer, int max_len);
char* get_password(char* buffer, int max_len);

#endif
