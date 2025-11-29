#include "lib/stdio.h"
#include "lib/syscalls.h"
#include "lib/string.h"

int fopen(char* filename, char* mode) {
    int fd = sys_open(filename, 0);
    
    // If open failed and mode implies creation (w, a, w+, a+), try to create it
    if (fd < 0 && mode && (strchr(mode, 'w') || strchr(mode, 'a'))) {
        sys_mkfile(filename, "");
        fd = sys_open(filename, 0);
    }
    
    return fd;
}

void fclose(int fd) {
    sys_close(fd);
}

int fread(void* ptr, int size, int count, int fd) {
    int total_bytes = size * count;
    int bytes_read = sys_read(fd, ptr, total_bytes);
    if (bytes_read < 0) return 0;
    return bytes_read / size;
}

int fwrite(void* ptr, int size, int count, int fd) {
    int total_bytes = size * count;
    int bytes_written = sys_write(fd, ptr, total_bytes);
    if (bytes_written < 0) return 0;
    return bytes_written / size;
}
