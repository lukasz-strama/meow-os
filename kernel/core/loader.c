#include "core/loader.h"
#include "fs/fat.h"
#include "core/gdt.h"
#include "drivers/print.h"
#include "core/process.h"

int program_load(char* filename) {
    void* entry_point = (void*)0x400000;
    
    // printf("Loading %s...\n", filename);
    
    if (fat_read_file_to_buffer(filename, (char*)entry_point, 1024 * 64)) {
        // Update Process Name
        if (current_process) {
            int i = 0;
            // Extract just the filename if path is provided (e.g. /BIN/SHELL.BIN -> SHELL.BIN)
            char* name_start = filename;
            char* p = filename;
            while (*p) {
                if (*p == '/') name_start = p + 1;
                p++;
            }
            
            while (i < 31 && name_start[i]) {
                current_process->name[i] = name_start[i];
                i++;
            }
            current_process->name[i] = '\0';
        }

        // Ensure GDT is correct for User Mode
        // extern void fix_gdt(); // Declared in core/gdt.h usually, or we can declare it here if missing
        void fix_gdt(); // It is in gdt_fix.c, often not in header.
        fix_gdt();
        
        // printf("Jumping to User Mode...\n");
        enter_user_mode((uint64_t)entry_point, 0x500000);
        return 0; // Should not be reached
    }
    
    return -1; // Failed
}
