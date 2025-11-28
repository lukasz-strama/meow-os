#include "lib/stdio.h"
#include "lib/string.h"
#include "lib/syscalls.h"

int str_starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) {
            return 0;
        }
    }
    return 1;
}

void main() {
    char cmd[100];
    char user[32];
    
    sys_get_user(user);
    if (user[0] == '\0') {
        sys_clear();
        sys_set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("Access Denied: No active session.\n");
        sys_set_color(COLOR_WHITE, COLOR_BLACK);
        sys_exec("LOGIN.BIN");
        return;
    }

    sys_clear();
    sys_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
    printf("\n--- MeowSH v0.3 (User Mode) ---\n");
    sys_set_color(COLOR_WHITE, COLOR_BLACK);

    while (1) {
        sys_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("%s", user);
        sys_set_color(COLOR_WHITE, COLOR_BLACK);
        printf("@");
        sys_set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("meowos");
        sys_set_color(COLOR_WHITE, COLOR_BLACK);
        printf(" $ ");
        
        gets(cmd, 100);

        if (strcmp(cmd, "help") == 0) {
            printf("Available Commands:\n");
            printf("  help      - Show this list\n");
            printf("  clear     - Clear the screen\n");
            printf("  echo      - Print text\n");
            printf("  ls        - List files\n");
            printf("  cat       - Read file content\n");
            printf("  mkfile    - Create a new file\n");
            printf("  rm        - Delete a file\n");
            printf("  nano      - Text Editor\n");
            printf("  snake     - Play Snake game\n");
            printf("  logout    - Logout current user\n");
            printf("  reboot    - Restart system\n");
            printf("  shutdown  - Power off system\n");
            printf("  kmonitor  - Enter Kernel Monitor\n");
        } else if (strcmp(cmd, "clear") == 0) {
            sys_clear();
        } else if (strncmp(cmd, "echo ", 5) == 0) {
            printf("%s\n", cmd + 5);
        } else if (strcmp(cmd, "echo") == 0) {
            printf("\n");
        } else if (strcmp(cmd, "ls") == 0) {
            sys_ls();
        } else if (str_starts_with(cmd, "cat ")) {
            sys_cat(cmd + 4);
        } else if (str_starts_with(cmd, "rm ")) {
            sys_rm(cmd + 3);
        } else if (str_starts_with(cmd, "mkfile ")) {
            char* args = cmd + 7;
            char* filename = args;
            char* content = 0;
            
            // Find space separator
            int i = 0;
            while (args[i]) {
                if (args[i] == ' ') {
                    args[i] = '\0'; // Terminate filename
                    content = args + i + 1;
                    break;
                }
                i++;
            }
            
            if (content) {
                sys_mkfile(filename, content);
            } else {
                printf("Usage: mkfile <filename> <content>\n");
            }
        } else if (strcmp(cmd, "snake") == 0) {
            sys_exec("SNAKE.BIN");
            printf("Failed to launch snake.\n");
        } else if (strcmp(cmd, "nano") == 0) {
            sys_exec("NANO.BIN");
            printf("Failed to launch nano.\n");
        } else if (strcmp(cmd, "kmonitor") == 0) {
            sys_kmonitor();
        } else if (strcmp(cmd, "logout") == 0) {
            sys_exec("LOGIN.BIN");
        } else if (strcmp(cmd, "reboot") == 0) {
            sys_reboot();
        } else if (strcmp(cmd, "shutdown") == 0) {
            sys_shutdown();
        } else if (cmd[0] != '\0') {
            printf("Unknown command: %s\n", cmd);
        }
    }
}
