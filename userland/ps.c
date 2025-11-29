#include "lib/stdio.h"
#include "lib/syscalls.h"
#include "lib/stats.h"

void main(int argc, char** argv) {
    ProcessInfo info;
    printf("PID  STATE      NAME\n");
    printf("---  ---------  ----------------\n");

    // We'll scan PIDs 0 to 100 for now
    for (int i = 0; i < 100; i++) {
        if (sys_get_proc_info(i, &info) == 0) {
            char* state_str = "UNKNOWN";
            switch (info.state) {
                case 0: state_str = "READY"; break;
                case 1: state_str = "RUNNING"; break;
                case 2: state_str = "BLOCKED"; break;
                case 3: state_str = "TERMINATED"; break;
            }
            
            // Simple formatting
            printf("%d", info.pid);
            
            // Padding for PID
            if (info.pid < 10) printf("    ");
            else if (info.pid < 100) printf("   ");
            else printf("  ");

            printf("%s", state_str);

            // Padding for State
            int len = 0;
            char* s = state_str;
            while(*s++) len++;
            
            for(int k=0; k < 11 - len; k++) printf(" ");

            printf("%s\n", info.name);
        }
    }

    printf("\nPress any key to exit...");
    sys_getch();
}
