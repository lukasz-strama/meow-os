#include "lib/stdio.h"
#include "lib/string.h"
#include "lib/syscalls.h"

void main() {
    char user[32];
    char pass[32];

    sys_clear();
    printf("\n--- MeowOS v0.3 Login ---\n");

    while (1) {
        printf("\nUser: ");
        gets(user, 32);

        printf("Password: ");
        get_password(pass, 32);

        if (strcmp(user, "lukasz") == 0 && strcmp(pass, "123") == 0) {
            printf("\nLogin Successful! Loading Shell...\n");
            sys_login(user);
            sys_exec("SHELL.BIN");
            printf("Error: Failed to load SHELL.BIN\n");
        } else if (strcmp(user, "kmonitor") == 0 && strcmp(pass, "root") == 0) {
            printf("\nEntering Kernel Monitor...\n");
            sys_kmonitor();
        } else {
            printf("\nAccess Denied.\n");
        }
    }
}
