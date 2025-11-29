#include "lib/stdio.h"
#include "lib/syscalls.h"

int main(int argc, char** argv) {
    printf("Received %d arguments:\n", argc);
    for (int i = 0; i < argc; i++) {
        printf("argv[%d] = '%s'\n", i, argv[i]);
    }
    
    printf("\nPress any key to exit...");
    sys_getch();
    return 0;
}
