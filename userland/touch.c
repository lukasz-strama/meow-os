#include "lib/stdio.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: touch <file>\n");
        return 1;
    }

    printf("Creating file: %s\n", argv[1]);
    int fd = fopen(argv[1], "w");
    if (fd < 0) {
        printf("Failed to create file: %s\n", argv[1]);
        return 1;
    }

    fclose(fd);
    printf("File created successfully.\n");
    return 0;
}
