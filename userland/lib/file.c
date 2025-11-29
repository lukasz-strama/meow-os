#include "lib/stdio.h"
#include "lib/syscalls.h"

int fopen(char* filename, char* mode) {
    // Mode is ignored for now, assuming read/write based on flags if we had them.
    // For now, just open.
    return sys_open(filename, 0);
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
