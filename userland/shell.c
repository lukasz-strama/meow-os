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

void to_upper(char* str) {
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str -= 32;
        }
        str++;
    }
}

void main(int argc, char** argv) {
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

    // sys_clear(); // Removed to preserve output from executed commands
    
    int show_banner = 1;
    if (argc > 1 && strcmp(argv[1], "--reload") == 0) {
        show_banner = 0;
    }

    if (show_banner) {
        sys_set_color(COLOR_LIGHT_CYAN, COLOR_BLACK);
        printf("\n--- MeowSH v0.3 (User Mode) ---\n");
        sys_set_color(COLOR_WHITE, COLOR_BLACK);
    }

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

        // Redirection Logic
        char* redir_ptr = strchr(cmd, '>');
        int redirect_fd = -1;
        char* redirect_file = 0;

        if (redir_ptr) {
            *redir_ptr = '\0'; // Split command
            redirect_file = redir_ptr + 1;
            while (*redirect_file == ' ') redirect_file++; // Trim leading spaces
            
            // Trim trailing spaces from cmd
            int len = strlen(cmd);
            while (len > 0 && cmd[len-1] == ' ') {
                cmd[len-1] = '\0';
                len--;
            }

            // Perform Swap
            sys_close(stdout); // Close stdout
            
            char abs_redir_path[256];
            get_abs_path(redirect_file, abs_redir_path);
            
            redirect_fd = fopen(abs_redir_path, "w"); // Should be stdout
            if (redirect_fd != stdout) {
                // Failed to get FD stdout. Restore console.
                if (redirect_fd >= 0) sys_close(redirect_fd);
                fopen("/dev/console", "w");
                printf("Redirection failed: FD %d (Expected %d)\n", redirect_fd, stdout);
                continue;
            }
        }

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
            printf("  ps        - List processes\n");
            printf("  free      - Show memory usage\n");
            printf("  logout    - Logout current user\n");
            printf("  reboot    - Restart system\n");
            printf("  shutdown  - Power off system\n");
            printf("  kmonitor  - Enter Kernel Monitor\n");
        } else if (strcmp(cmd, "clear") == 0) {
            sys_clear();
        } else if (strcmp(cmd, "ls") == 0) {
            // sys_ls(cwd); // Old kernel-side ls
            
            printf("Directory listing for %s:\n", cwd);
            char name[32];
            unsigned int size;
            int is_dir;
            int i = 0;
            
            while (sys_read_dir(cwd, i, name, &size, &is_dir) == 0) {
                if (is_dir) {
                    sys_set_color(COLOR_LIGHT_BLUE, COLOR_BLACK);
                    printf("  [DIR] %s\n", name);
                    sys_set_color(COLOR_WHITE, COLOR_BLACK);
                } else {
                    printf("  %s (%d bytes)\n", name, size);
                }
                i++;
            }
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
            if (sys_exec("/BIN/SNAKE.BIN") != 0) {
                printf("Failed to launch snake.\n");
            }
        } else if (str_starts_with(cmd, "nano") && (cmd[4] == ' ' || cmd[4] == '\0')) {
            char* args = NULL;
            if (cmd[4] == ' ') {
                args = cmd + 5;
            }

            char exec_cmd[128];
            if (args) {
                strcpy(exec_cmd, "/BIN/NANO.BIN ");
                strcat(exec_cmd, args);
            } else {
                strcpy(exec_cmd, "/BIN/NANO.BIN");
            }

            if (sys_exec(exec_cmd) != 0) {
                printf("Failed to launch nano.\n");
            }
        } else if (strcmp(cmd, "ps") == 0) {
            if (sys_exec("/BIN/PS.BIN") != 0) {
                printf("Failed to launch ps.\n");
            }
        } else if (strcmp(cmd, "free") == 0) {
            if (sys_exec("/BIN/FREE.BIN") != 0) {
                printf("Failed to launch free.\n");
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
            // Auto-exec fallback
            char bin_name[32];
            char exec_cmd[128];
            
            // Extract first word
            int i = 0;
            while (cmd[i] && cmd[i] != ' ' && i < 31) {
                bin_name[i] = cmd[i];
                i++;
            }
            bin_name[i] = '\0';
            
            to_upper(bin_name);
            
            // Construct /BIN/NAME.BIN
            strcpy(exec_cmd, "/BIN/");
            strcat(exec_cmd, bin_name);
            strcat(exec_cmd, ".BIN");
            
            // Append arguments if any
            char* args = strchr(cmd, ' ');
            if (args) {
                strcat(exec_cmd, args);
            }
            
            if (sys_exec(exec_cmd) != 0) {
                printf("Unknown command: %s\n", cmd);
            }
        }
        
        // Restore stdout
        if (redirect_file) {
            sys_close(redirect_fd);
            fopen("/dev/console", "w"); // Should be 1
        }
    }
}
