#include "kmonitor/editor.h"
#include "drivers/print.h"
#include "drivers/io.h"
#include "drivers/keyboard.h"
#include "fs/fat.h"

extern size_t row;
extern size_t col;

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
        int length = 0;
        while (buffer[length]) length++;
        printf("--- EDITING: %s (ESC: Save&Quit) ---   Length: %d\n", filename, length);

        // Content (Black background)
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        // Print with line numbers
        size_t cursor_screen_row = 0;
        size_t cursor_screen_col = 0;
        int line_num = 1;
        int buf_index = 0;
        printf("%d | ", line_num);
        while (buffer[buf_index]) {
            if (buf_index == cursor_pos) {
                cursor_screen_row = row;
                cursor_screen_col = col;
            }
            if (buffer[buf_index] == '\n') {
                print_char('\n');
                line_num++;
                printf("%d | ", line_num);
            } else {
                print_char(buffer[buf_index]);
            }
            buf_index++;
        }
        if (buf_index == cursor_pos) {
            cursor_screen_row = row;
            cursor_screen_col = col;
        }

        // Set hardware cursor position
        uint16_t pos = cursor_screen_row * 80 + cursor_screen_col;
        outb(0x3D4, 0x0F);
        outb(0x3D5, (uint8_t)(pos & 0xFF));
        outb(0x3D4, 0x0E);
        outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));

        // 3. Input
        char c = keyboard_get_char();

        // 4. Logic
        if (c == 0x1B) { // ESC
            // Ask to save
            print_str("\nSave changes? (y/n): ");
            char choice = keyboard_get_char();
            if (choice == 'y' || choice == 'Y') {
                // Save
                fat_delete_file(filename);
                fat_create_file(filename, buffer);
            }
            // Exit
            break;
        }
        else if (c == '\x80') { // Left arrow
            if (cursor_pos > 0) cursor_pos--;
        }
        else if (c == '\x81') { // Right arrow
            if (buffer[cursor_pos] != 0) cursor_pos++;
        }
        else if (c == '\b') { // Backspace
            if (cursor_pos > 0) {
                // Shift left from cursor_pos - 1
                for (int i = cursor_pos - 1; buffer[i]; i++) {
                    buffer[i] = buffer[i + 1];
                }
                cursor_pos--;
            }
        }
        else if (c >= 32 && c <= 126) { // Printable characters
            if (cursor_pos < EDITOR_BUFFER_SIZE - 1) {
                // Shift right from cursor_pos
                for (int i = EDITOR_BUFFER_SIZE - 2; i >= cursor_pos; i--) {
                    buffer[i + 1] = buffer[i];
                }
                buffer[cursor_pos] = c;
                cursor_pos++;
            }
        }
        else if (c == '\n') { // Enter
            if (cursor_pos < EDITOR_BUFFER_SIZE - 1) {
                // Shift right from cursor_pos
                for (int i = EDITOR_BUFFER_SIZE - 2; i >= cursor_pos; i--) {
                    buffer[i + 1] = buffer[i];
                }
                buffer[cursor_pos] = '\n';
                cursor_pos++;
            }
        }
    }

    // 5. Exit (save already handled)
    // CRITICAL: Restore Shell Colors and Clear for clean exit
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK); // Default shell color
    print_clear();
}
