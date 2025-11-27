#include "kmonitor/editor.h"
#include "drivers/print.h"
#include "drivers/io.h"
#include "drivers/keyboard.h"
#include "fs/fat.h"

#define EDITOR_BUFFER_SIZE 1024

static char buffer[EDITOR_BUFFER_SIZE];
static int cursor_pos = 0;

void editor_draw_ui(char* filename) {
    print_clear();
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLUE);
    print_str("--- EDITING: ");
    print_str(filename);
    print_str(" (ESC to Quit/Save) ---\n");
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    
    print_str(buffer);
}

void editor_start(char* filename) {
    // 1. Initialize
    print_clear();
    
    // Initialize buffer
    for (int i = 0; i < EDITOR_BUFFER_SIZE; i++) buffer[i] = 0;
    cursor_pos = 0;

    // Try to load existing file
    if (fat_read_file_to_buffer(filename, buffer, EDITOR_BUFFER_SIZE)) {
        // Calculate cursor position (end of file)
        while (buffer[cursor_pos] != 0 && cursor_pos < EDITOR_BUFFER_SIZE) {
            cursor_pos++;
        }
    }

    while (1) {
        // 2. Render UI (Redraw Strategy)
        print_clear();

        // Header (Blue background)
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLUE);
        printf("--- EDITING: %s (ESC: Save&Quit) ---   Length: %d\n", filename, cursor_pos);

        // Content (Black background)
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        printf("%s", buffer); // Print the whole buffer

        // Draw cursor (simple underscore)
        print_char('_');

        // Hide hardware cursor to avoid double cursor
        outb(0x3D4, 0x0F);
        outb(0x3D5, 0xD0);  // Low byte of 2000 (0x7D0)
        outb(0x3D4, 0x0E);
        outb(0x3D5, 0x07);  // High byte of 2000

        // 3. Input
        char c = keyboard_get_char();

        // 4. Logic
        if (c == 0x1B) { // ESC
            break; 
        }
        else if (c == '\b') { // Backspace
            if (cursor_pos > 0) {
                buffer[--cursor_pos] = '\0';
            }
        }
        else { // Regular Char
            if (cursor_pos < EDITOR_BUFFER_SIZE - 1 && c >= 32 && c <= 126) { // Only printable chars
                buffer[cursor_pos++] = c;
                buffer[cursor_pos] = '\0';
            }
            // Handle Enter key
            if (c == '\n' && cursor_pos < EDITOR_BUFFER_SIZE - 1) {
                 buffer[cursor_pos++] = '\n';
                 buffer[cursor_pos] = '\0';
            }
        }
    }

    // 5. Save & Exit
    // Delete existing file first (simple overwrite strategy)
    fat_delete_file(filename);
    fat_create_file(filename, buffer);

    // CRITICAL: Restore Shell Colors and Clear for clean exit
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK); // Default shell color
    print_clear();
}
