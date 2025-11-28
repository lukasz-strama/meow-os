#include "lib/stdio.h"
#include "lib/string.h"
#include "lib/syscalls.h"

void main() {
    char cmd[100];
    sys_clear();
    printf("\n--- MeowSH v0.1 (User Mode) ---\n");

    while (1) {
        printf("user@meowos $ ");
        gets(cmd, 100);

        if (strcmp(cmd, "help") == 0) {
            printf("Available: help, clear, echo, snake, exit\n");
        } else if (strcmp(cmd, "clear") == 0) {
            sys_clear();
        } else if (strncmp(cmd, "echo ", 5) == 0) {
            printf("%s\n", cmd + 5);
        } else if (strcmp(cmd, "echo") == 0) {
            printf("\n");
        } else if (strcmp(cmd, "snake") == 0) {
            sys_exec("SNAKE.BIN");
            printf("Failed to launch snake.\n");
        } else if (strcmp(cmd, "exit") == 0) {
            sys_exit(0);
        } else if (cmd[0] != '\0') {
            printf("Unknown command: %s\n", cmd);
        }
    }
}
