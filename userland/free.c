#include "lib/stdio.h"
#include "lib/syscalls.h"
#include "lib/stats.h"

void main() {
    MemInfo info;
    sys_get_mem_info(&info);

    printf("              total        used        free\n");
    
    // Convert to KB for display
    unsigned int total_kb = info.total_ram / 1024;
    unsigned int used_kb = info.used_ram / 1024;
    unsigned int free_kb = info.free_ram / 1024;

    printf("Mem:       %d KB    %d KB    %d KB\n", total_kb, used_kb, free_kb);

    printf("\nPress any key to exit...");
    sys_getch();
}
