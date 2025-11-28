#include "lib/stdio.h"
#include "lib/syscalls.h"
#include "lib/string.h"

void main() {
    printf("MeowLib initialized!\n");

    char buf[20];
    strcpy(buf, "Copy Test");
    printf("String copy result: %s\n", buf);

    int a = 10, b = 20;
    printf("Math test: %d + %d = %d\n", a, b, a+b);

    printf("Exiting cleanly now...\n");
    sys_exit(0);
}
