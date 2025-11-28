#include "lib/stdio.h"
#include "lib/string.h"
#include "lib/syscalls.h"

char cwd[256] = "/";

int str_starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) {
            return 0;
        }
    }
    return 1;
}

void get_abs_path(char* input, char* output) {
    if (input[0] == '/') {
        strcpy(output, input);
    } else {
        strcpy(output, cwd);
        if (output[strlen(output)-1] != '/') strcat(output, "/");
        strcat(output, input);
    }
}

void handle_cd(char* path) {
    char temp[256];
    if (strcmp(path, "/") == 0) {
        strcpy(cwd, "/");
        return;
    }
    if (strcmp(path, "..") == 0) {
        int len = strlen(cwd);
        if (len > 1) {
            int i = len - 1;
            while (i > 0 && cwd[i] != '/') i--;
            if (i == 0) cwd[1] = '\0';
            else cwd[i] = '\0';
        }
        return;
    }
    
    get_abs_path(path, temp);
    
    int is_dir;
    if (sys_stat(temp, 0, &is_dir) == 0 && is_dir) {
        strcpy(cwd, temp);
    } else {
        printf("Directory not found: %s\n", temp);
    }
}

void main() {
    char cmd[100];
    char user[32];
    char abs_path[256];
    
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
    printf("\n--- MeowSH v0.4 (User Mode) ---\n");
    sys_set_color(COLOR_WHITE, COLOR_BLACK);

    while (1) {
        sys_set_color(COLOR_LIGHT_GREEN, COLOR_BLACK);
        printf("%s", user);
        sys_set_color(COLOR_WHITE, COLOR_BLACK);
        printf("@");
        sys_set_color(COLOR_LIGHT_RED, COLOR_BLACK);
        printf("meowos");
        sys_set_color(COLOR_WHITE, COLOR_BLACK);
        printf(" %s $ ", cwd);
        
        gets(cmd, 100);

        if (strcmp(cmd, "help") == 0) {
            printf("Available Commands:\n");
            printf("  help      - Show this list\n");
            printf("  clear     - Clear the screen\n");
            printf("  echo      - Print text\n");
            printf("  ls        - List files\n");
            printf("  cd        - Change directory\n");
            printf("  mkdir     - Create directory\n");
            printf("  rmdir     - Remove directory\n");
            printf("  exec      - Execute a program\n");
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
            sys_ls(cwd);
        } else if (str_starts_with(cmd, "cd ")) {
            handle_cd(cmd + 3);
        } else if (str_starts_with(cmd, "mkdir ")) {
            get_abs_path(cmd + 6, abs_path);
            sys_mkdir(abs_path);
        } else if (str_starts_with(cmd, "rmdir ")) {
            get_abs_path(cmd + 6, abs_path);
            sys_rmdir(abs_path);
        } else if (str_starts_with(cmd, "exec ")) {
            char* prog = cmd + 5;
            if (sys_exec(prog) != 0) {
                char bin_path[128];
                strcpy(bin_path, "/BIN/");
                strcat(bin_path, prog);
                if (sys_exec(bin_path) != 0) {
                    printf("Command not found: %s\n", prog);
                }
            }
        } else if (str_starts_with(cmd, "cat ")) {
            get_abs_path(cmd + 4, abs_path);
            sys_cat(abs_path);
        } else if (str_starts_with(cmd, "rm ")) {
            get_abs_path(cmd + 3, abs_path);
            sys_rm(abs_path);
        } else if (str_starts_with(cmd, "mkfile ")) {
            char* args = cmd + 7;
            char* filename = args;
            char* content = 0;
            
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
                get_abs_path(filename, abs_path);
                sys_mkfile(abs_path, content);
            } else {
                printf("Usage: mkfile <filename> <content>\n");
            }
        } else if (strcmp(cmd, "snake") == 0) {
            if (sys_exec("SNAKE.BIN") != 0) {
                if (sys_exec("/BIN/SNAKE.BIN") != 0) {
                    printf("Failed to launch snake.\n");
                }
            }
        } else if (strcmp(cmd, "nano") == 0) {
            if (sys_exec("NANO.BIN") != 0) {
                if (sys_exec("/BIN/NANO.BIN") != 0) {
                    printf("Failed to launch nano.\n");
                }
            }
        } else if (strcmp(cmd, "kmonitor") == 0) {
            sys_kmonitor();
        } else if (strcmp(cmd, "logout") == 0) {
            if (sys_exec("LOGIN.BIN") != 0) {
                sys_exec("/BIN/LOGIN.BIN");
            }
        } else if (strcmp(cmd, "reboot") == 0) {
            sys_reboot();
        } else if (strcmp(cmd, "shutdown") == 0) {
            sys_shutdown();
        } else if (cmd[0] != '\0') {
            printf("Unknown command: %s\n", cmd);
        }
    }
}
