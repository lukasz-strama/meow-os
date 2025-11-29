#include "lib/stdio.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: cat <file>\n");
        return 1;
    }

    int fd = fopen(argv[1], "r");
    if (fd < 0) {
        printf("File not found: %s\n", argv[1]);
        return 1;
    }

    char buf[64];
    int n;
    while ((n = fread(buf, 1, 64, fd)) > 0) {
        for (int i = 0; i < n; i++) {
            putchar(buf[i]);
        }
    }
    
    fclose(fd);
    return 0;
}
