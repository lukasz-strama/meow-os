#include "drivers/print.h"
#include "drivers/io.h"

const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

struct Char* buffer = (struct Char*) 0xb8000;
size_t col = 0;
size_t row = 0;
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;

void print_update_cursor() {
    uint16_t pos = row * NUM_COLS + col;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t) (pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void print_clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
}

void print_clear() {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        print_clear_row(i);
    }
    col = 0;
    row = 0;
    print_update_cursor();
}

void print_newline() {
    col = 0;

    if (row < NUM_ROWS - 1) {
        row++;
        return;
    }

    for (size_t row = 1; row < NUM_ROWS; row++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * row];
            buffer[col + NUM_COLS * (row - 1)] = character;
        }
    }

    print_clear_row(NUM_ROWS - 1);
}

void print_char(char character) {
    if (character == '\n') {
        print_newline();
        print_update_cursor();
        return;
    }

    if (col > NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    col++;
    print_update_cursor();
}

void print_str(char* string) {
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) string[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

void print_backspace() {
    if (col == 0 && row == 0) {
        return;
    }

    if (col == 0) {
        row--;
        col = NUM_COLS;
    }

    col--;
    buffer[col + NUM_COLS * row] = (struct Char) {
        character: ' ',
        color: color,
    };
    print_update_cursor();
}

void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
}

void print_hex(uint64_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    print_str("0x");
    for (int i = 15; i >= 0; i--) {
        print_char(hex_chars[(num >> (i * 4)) & 0xF]);
    }
}

static void print_int_helper(int num) {
    char buffer[20];
    int i = 0;
    int is_negative = 0;

    if (num == 0) {
        print_char('0');
        return;
    }

    if (num < 0) {
        is_negative = 1;
        num = -num;
    }

    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    if (is_negative) {
        print_char('-');
    }

    while (i > 0) {
        print_char(buffer[--i]);
    }
}

static void print_hex_helper(uint64_t num) {
    char hex_chars[] = "0123456789ABCDEF";
    char buffer[20];
    int i = 0;

    if (num == 0) {
        print_char('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = hex_chars[num % 16];
        num /= 16;
    }

    while (i > 0) {
        print_char(buffer[--i]);
    }
}

void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] != '%') {
            print_char(format[i]);
            continue;
        }

        i++; // Move past '%'
        switch (format[i]) {
            case 'c': {
                char c = (char)va_arg(args, int);
                print_char(c);
                break;
            }
            case 's': {
                char* s = va_arg(args, char*);
                if (s == NULL) s = "(null)";
                print_str(s);
                break;
            }
            case 'd': {
                int d = va_arg(args, int);
                print_int_helper(d);
                break;
            }
            case 'x': {
                unsigned int x = va_arg(args, unsigned int);
                print_hex_helper((uint64_t)x);
                break;
            }
            case 'p': {
                void* p = va_arg(args, void*);
                print_str("0x");
                print_hex_helper((uint64_t)p);
                break;
            }
            case '%': {
                print_char('%');
                break;
            }
            default: {
                print_char('%');
                print_char(format[i]);
                break;
            }
        }
    }

    va_end(args);
}
