#include "core/loader.h"
#include "fs/fat.h"
#include "core/gdt.h"
#include "drivers/print.h"
#include "core/process.h"
#include "core/elf.h"
#include "memory/heap.h"
#include "fs/vfs.h"

int load_elf(char* command_line);

// Helper functions
int loader_strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

void loader_strcpy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

void get_first_token(char* cmd, char* buf) {
    int i = 0;
    while (cmd[i] && cmd[i] != ' ') {
        buf[i] = cmd[i];
        i++;
    }
    buf[i] = '\0';
}

int program_load(char* command_line) {
    // Close all open files from previous process to prevent FD leaks
    // Since we use a global FD table for now.
    // vfs_close_all(); // MOVED TO sys_exit to allow inheritance

    char filename[64];
    get_first_token(command_line, filename);

    // Check for ELF magic
    char magic[4];
    if (fat_read_file_to_buffer(filename, magic, 4)) {
        if (magic[0] == 0x7F && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F') {
            return load_elf(command_line);
        }
    }

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
        enter_user_mode((uint64_t)entry_point, 0x500000, 0, 0);
        return 0; // Should not be reached
    }
    
    return -1; // Failed
}

int load_elf(char* command_line) {
    char filename[64];
    get_first_token(command_line, filename);

    uint16_t cluster;
    uint32_t size;
    uint8_t is_dir;
    
    if (!fat_resolve_path(filename, &cluster, &size, &is_dir)) {
        printf("ELF: File not found: %s\n", filename);
        return -1;
    }
    
    uint8_t* file_buffer = (uint8_t*)malloc(size);
    if (!file_buffer) {
        printf("ELF: Memory allocation failed (%d bytes)\n", size);
        return -1;
    }
    
    if (!fat_read_file_to_buffer(filename, (char*)file_buffer, size)) {
        printf("ELF: Failed to read file\n");
        free(file_buffer);
        return -1;
    }
    
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)file_buffer;
    
    // Validate Magic
    if (ehdr->e_ident[0] != ELFMAG0 || ehdr->e_ident[1] != ELFMAG1 || 
        ehdr->e_ident[2] != ELFMAG2 || ehdr->e_ident[3] != ELFMAG3) {
        free(file_buffer);
        return -1; // Not ELF
    }
    
    // Validate Class (64-bit)
    if (ehdr->e_ident[4] != ELFCLASS64) {
        printf("ELF: Not 64-bit\n");
        free(file_buffer);
        return -1;
    }

    // Validate Endianness (Little Endian)
    if (ehdr->e_ident[5] != 1) { // ELFDATA2LSB
        printf("ELF: Not Little Endian\n");
        free(file_buffer);
        return -1;
    }

    // Validate Type (Executable)
    if (ehdr->e_type != 2) { // ET_EXEC
        printf("ELF: Not Executable (Type %d)\n", ehdr->e_type);
        free(file_buffer);
        return -1;
    }
    
    // Load Segments
    Elf64_Phdr* phdr = (Elf64_Phdr*)(file_buffer + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint8_t* dest = (uint8_t*)phdr[i].p_vaddr;
            uint8_t* src = file_buffer + phdr[i].p_offset;
            
            // Copy file data
            for (uint64_t j = 0; j < phdr[i].p_filesz; j++) {
                dest[j] = src[j];
            }
            
            // Zero BSS
            if (phdr[i].p_memsz > phdr[i].p_filesz) {
                for (uint64_t j = phdr[i].p_filesz; j < phdr[i].p_memsz; j++) {
                    dest[j] = 0;
                }
            }
        }
    }
    
    uint64_t entry = ehdr->e_entry;
    free(file_buffer);
    
    // Update Process Name
    if (current_process) {
        int i = 0;
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

    void fix_gdt();
    fix_gdt();
    
    // --- Argument Passing Logic ---
    
    // Parse Args
    int argc = 0;
    char* args[32]; // Max 32 args
    char* ptr;
    int in_token = 0;
    
    char* cmd_copy = (char*)malloc(loader_strlen(command_line) + 1);
    loader_strcpy(cmd_copy, command_line);
    
    ptr = cmd_copy;
    while (*ptr) {
        if (*ptr != ' ') {
            if (!in_token) {
                if (argc < 32) args[argc++] = ptr;
                in_token = 1;
            }
        } else {
            *ptr = '\0'; // Terminate previous token
            in_token = 0;
        }
        ptr++;
    }
    
    uint64_t rsp = 0x500000;
    
    // Push Strings
    uint64_t* user_argv = (uint64_t*)malloc(sizeof(uint64_t) * argc);
    
    for (int i = 0; i < argc; i++) {
        int len = loader_strlen(args[i]);
        rsp -= (len + 1);
        loader_strcpy((char*)rsp, args[i]);
        user_argv[i] = rsp;
    }
    
    // Align RSP for argv array
    rsp -= (rsp % 8);
    
    // Push argv array (pointers)
    // argv[argc] = NULL
    rsp -= 8;
    *(uint64_t*)rsp = 0;
    
    for (int i = argc - 1; i >= 0; i--) {
        rsp -= 8;
        *(uint64_t*)rsp = user_argv[i];
    }
    
    uint64_t argv_addr = rsp;
    
    free(cmd_copy);
    free(user_argv);

    // printf("ELF: Jumping to entry point %x (argc=%d)\n", entry, argc);
    enter_user_mode(entry, rsp, argc, argv_addr);
    return 0;
}
