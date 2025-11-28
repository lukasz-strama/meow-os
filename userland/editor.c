#include "lib/stdio.h"
#include "lib/string.h"
#include "lib/syscalls.h"

#define EDITOR_BUFFER_SIZE 2048
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

char buffer[EDITOR_BUFFER_SIZE];
int cursor_pos = 0;
int buffer_len = 0;
char filename[32];

void draw_ui() {
    sys_clear();

    // 1. Title Bar
    sys_gotoxy(0, 0);
    sys_set_color(COLOR_BLACK, COLOR_LIGHT_GRAY);
    printf("  MeowNano 1.0   File: %-20s                                    ", filename);
    
    // 2. Text Area
    sys_set_color(COLOR_WHITE, COLOR_BLACK);
    sys_gotoxy(0, 1);
    
    int row = 1;
    int col = 0;
    int cursor_screen_x = 0;
    int cursor_screen_y = 1;

    for (int i = 0; i < buffer_len; i++) {
        if (i == cursor_pos) {
            cursor_screen_x = col;
            cursor_screen_y = row;
        }

        if (buffer[i] == '\n') {
            printf("\n");
            row++;
            col = 0;
        } else {
            printf("%c", buffer[i]);
            col++;
        }

        // Wrap text if needed (simple wrapping)
        if (col >= SCREEN_WIDTH) {
            row++;
            col = 0;
        }
    }

    if (cursor_pos == buffer_len) {
        cursor_screen_x = col;
        cursor_screen_y = row;
    }

    // 3. Status Bar
    sys_gotoxy(0, SCREEN_HEIGHT - 1);
    sys_set_color(COLOR_BLACK, COLOR_LIGHT_GRAY);
    printf(" [ESC] Save & Exit                                                      ");

    // 4. Set Cursor
    sys_gotoxy(cursor_screen_x, cursor_screen_y);
    sys_set_color(COLOR_WHITE, COLOR_BLACK);
}

void main() {
    // Parse arguments (simple hack: we don't have argc/argv yet, so we ask for filename if not provided? 
    // Actually, shell doesn't pass args to main yet. We need to fix that or ask for filename.)
    // For now, let's ask for filename.
    
    sys_clear();
    printf("Filename to edit: ");
    gets(filename, 32);

    // Load file
    buffer_len = 0;
    for (int i = 0; i < EDITOR_BUFFER_SIZE; i++) buffer[i] = 0;

    if (sys_read_file_content(filename, buffer, EDITOR_BUFFER_SIZE)) {
        // Calculate length
        while (buffer[buffer_len]) buffer_len++;
        cursor_pos = buffer_len; // Start at end? Or beginning? Let's start at 0.
        cursor_pos = 0;
    } else {
        // New file
        buffer_len = 0;
        cursor_pos = 0;
    }

    while (1) {
        draw_ui();

        char c = sys_getch();

        if (c == 0x1B) { // ESC
            sys_gotoxy(0, SCREEN_HEIGHT - 1);
            sys_set_color(COLOR_WHITE, COLOR_RED);
            printf(" Save changes? (y/n): ");
            char choice = sys_getch();
            if (choice == 'y' || choice == 'Y') {
                sys_mkfile(filename, buffer);
            }
            sys_set_color(COLOR_WHITE, COLOR_BLACK);
            sys_clear();
            sys_exec("SHELL.BIN");
            return;
        }
        else if (c == '\b') { // Backspace
            if (cursor_pos > 0) {
                for (int i = cursor_pos - 1; i < buffer_len; i++) {
                    buffer[i] = buffer[i + 1];
                }
                buffer_len--;
                cursor_pos--;
            }
        }
        else if (c == '\x80') { // Left
            if (cursor_pos > 0) cursor_pos--;
        }
        else if (c == '\x81') { // Right
            if (cursor_pos < buffer_len) cursor_pos++;
        }
        // Up/Down not implemented yet (needs line logic)
        else if (c >= 32 && c <= 126) { // Printable
            if (buffer_len < EDITOR_BUFFER_SIZE - 1) {
                for (int i = buffer_len; i > cursor_pos; i--) {
                    buffer[i] = buffer[i - 1];
                }
                buffer[cursor_pos] = c;
                buffer_len++;
                cursor_pos++;
            }
        }
        else if (c == '\n') { // Enter
            if (buffer_len < EDITOR_BUFFER_SIZE - 1) {
                for (int i = buffer_len; i > cursor_pos; i--) {
                    buffer[i] = buffer[i - 1];
                }
                buffer[cursor_pos] = '\n';
                buffer_len++;
                cursor_pos++;
            }
        }
    }
}
