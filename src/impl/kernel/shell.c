#include "shell.h"
#include "print.h"
#include "keyboard.h"
#include "heap.h"
#include "ata.h"
#include "fat.h"
#include "editor.h"

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int str_starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) {
            return 0;
        }
    }
    return 1;
}

void gets(char* buffer, int max_len) {
    int i = 0;
    while (1) {
        char c = keyboard_get_char();
        
        if (c == '\n') {
            print_char('\n');
            buffer[i] = '\0';
            return;
        } else if (c == '\b') {
            if (i > 0) {
                print_backspace();
                i--;
            }
        } else {
            if (i < max_len - 1) {
                print_char(c);
                buffer[i] = c;
                i++;
            }
        }
    }
}

void shell_init() {
    print_str("\nWelcome to MeowOS v0.1\n");
    print_str("Type 'help' for commands.\n");

    char cmd_buf[100];

    while (1) {
        print_str("MeowShell> ");
        gets(cmd_buf, 100);

        if (strcmp(cmd_buf, "help") == 0) {
            print_str("Available commands:\n");
            print_str("  help        - Show this message\n");
            print_str("  clear       - Clear screen\n");
            print_str("  info        - Show system info\n");
            print_str("  malloc_test - Run malloc demo\n");
            print_str("  read_disk   - Read first sector of disk\n");
            print_str("  write <msg> - Write message to disk\n");
            print_str("  ls          - List files\n");
            print_str("  cat <file>  - Read file content\n");
            print_str("  mkfile <f> <t> - Create file with text\n");
            print_str("  rm <file>   - Delete file\n");
            print_str("  edit <file> - Edit file\n");
        } else if (strcmp(cmd_buf, "clear") == 0) {
            print_clear();
        } else if (strcmp(cmd_buf, "ls") == 0) {
            fat_ls();
        } else if (str_starts_with(cmd_buf, "cat ")) {
            fat_read_file(cmd_buf + 4);
        } else if (str_starts_with(cmd_buf, "mkfile ")) {
            char* args = cmd_buf + 7;
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
                fat_create_file(filename, content);
            } else {
                print_str("Usage: mkfile <filename> <content>\n");
            }
        } else if (str_starts_with(cmd_buf, "rm ")) {
            fat_delete_file(cmd_buf + 3);
        } else if (str_starts_with(cmd_buf, "edit ")) {
            editor_start(cmd_buf + 5);
        } else if (strcmp(cmd_buf, "info") == 0) {
            print_str("MeowOS v0.1 - Barebones x86_64\n");
        } else if (strcmp(cmd_buf, "malloc_test") == 0) {
            void* ptr = malloc(128);
            printf("Allocated 128 bytes at %p\n", ptr);
            free(ptr);
            printf("Freed memory at %p\n", ptr);
        } else if (str_starts_with(cmd_buf, "write ")) {
            char* msg = cmd_buf + 6; // Skip "write "
            uint16_t* buf = (uint16_t*)malloc(512);
            if (!buf) {
                print_str("Failed to allocate buffer\n");
            } else {
                // Clear buffer
                uint8_t* byte_buf = (uint8_t*)buf;
                for (int i = 0; i < 512; i++) byte_buf[i] = 0;
                
                // Copy message
                int i = 0;
                while (msg[i] && i < 511) {
                    byte_buf[i] = msg[i];
                    i++;
                }
                
                print_str("Writing to Sector 0...\n");
                ata_write_sectors(0, 1, buf);
                print_str("Done.\n");
                free(buf);
            }
        } else if (strcmp(cmd_buf, "read_disk") == 0) {
            uint16_t* buf = (uint16_t*)malloc(512);
            if (!buf) {
                print_str("Failed to allocate buffer\n");
            } else {
                // Fill with dummy pattern to verify read
                uint8_t* byte_buf = (uint8_t*)buf;
                for (int i = 0; i < 512; i++) {
                    byte_buf[i] = 0xCC;
                }

                print_str("Reading Sector 0...\n");
                ata_read_sectors(0, 1, buf);
                
                // Print first 32 bytes (16 words)
                for (int i = 0; i < 16; i++) {
                    printf("%x ", buf[i]);
                }
                print_str("\n");
                
                // Print as string
                print_str("Text: ");
                byte_buf = (uint8_t*)buf;
                for (int i = 0; i < 64; i++) {
                    char c = byte_buf[i];
                    if (c >= 32 && c <= 126) {
                        char s[2] = {c, 0};
                        print_str(s);
                    } else {
                        print_str(".");
                    }
                }
                print_str("\n");

                free(buf);
            }
        } else if (cmd_buf[0] != '\0') {
            printf("Unknown command: %s\n", cmd_buf);
        }
    }
}