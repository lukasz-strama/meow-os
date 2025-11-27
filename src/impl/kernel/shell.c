#include "shell.h"
#include "print.h"
#include "keyboard.h"
#include "heap.h"
#include "ata.h"

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void gets(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        char c = keyboard_get_char();
        
        if (c == '\n') {
            buffer[i] = '\0';
            return;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
            }
        } else {
            buffer[i] = c;
            i++;
        }
    }
    buffer[i] = '\0';
}

void shell_init() {
    print_str("\nWelcome to MyOS v0.1\n");
    print_str("Type 'help' for commands.\n");

    char cmd_buf[100];

    while (1) {
        print_str("MyOS> ");
        gets(cmd_buf, 100);

        if (strcmp(cmd_buf, "help") == 0) {
            print_str("Available commands:\n");
            print_str("  help        - Show this message\n");
            print_str("  clear       - Clear screen\n");
            print_str("  info        - Show system info\n");
            print_str("  malloc_test - Run malloc demo\n");
            print_str("  read_disk   - Read first sector of disk\n");
        } else if (strcmp(cmd_buf, "clear") == 0) {
            print_clear();
        } else if (strcmp(cmd_buf, "info") == 0) {
            print_str("MyOS v0.1 - Barebones x86_64\n");
        } else if (strcmp(cmd_buf, "malloc_test") == 0) {
            void* ptr = malloc(128);
            printf("Allocated 128 bytes at %p\n", ptr);
            free(ptr);
            printf("Freed memory at %p\n", ptr);
        } else if (strcmp(cmd_buf, "read_disk") == 0) {
            uint16_t* buf = (uint16_t*)malloc(512);
            if (!buf) {
                print_str("Failed to allocate buffer\n");
            } else {
                print_str("Reading Sector 0...\n");
                ata_read_sectors(0, 1, buf);
                
                // Print first 32 bytes (16 words)
                for (int i = 0; i < 16; i++) {
                    printf("%x ", buf[i]);
                }
                print_str("\n");
                free(buf);
            }
        } else if (cmd_buf[0] != '\0') {
            printf("Unknown command: %s\n", cmd_buf);
        }
    }
}