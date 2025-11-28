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

        char buffer[128];
        int auth_success = 0;

        // Check /etc/passwd
        if (sys_read_file_content("/ETC/PASSWD", buffer, 128) > 0) {
            char file_user[32];
            char file_pass[32];
            int i = 0;
            int j = 0;
            
            // Read user
            while (buffer[i] && buffer[i] != ':' && j < 31) {
                file_user[j++] = buffer[i++];
            }
            file_user[j] = '\0';
            
            if (buffer[i] == ':') i++;
            
            // Read pass
            j = 0;
            while (buffer[i] && buffer[i] != '\n' && buffer[i] != '\r' && j < 31) {
                file_pass[j++] = buffer[i++];
            }
            file_pass[j] = '\0';

            if (strcmp(user, file_user) == 0 && strcmp(pass, file_pass) == 0) {
                auth_success = 1;
            }
        }

        if (auth_success) {
            printf("\nLogin Successful! Loading Shell...\n");
            sys_login(user);
            if (sys_exec("SHELL.BIN") != 0) {
                sys_exec("/BIN/SHELL.BIN");
            }
            printf("Error: Failed to load SHELL.BIN\n");
        } else if (strcmp(user, "kmonitor") == 0 && strcmp(pass, "root") == 0) {
            printf("\nEntering Kernel Monitor...\n");
            sys_kmonitor();
        } else {
            printf("\nAccess Denied.\n");
        }
    }
}
