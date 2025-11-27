#include "keyboard.h"
#include "print.h"
#include "io.h"
#include <stdbool.h>

static char buffer[256];
static uint8_t write_ptr = 0;
static uint8_t read_ptr = 0;
static bool shift_pressed = false;

// US QWERTY Lowercase
char kbd_us_lowercase[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

// US QWERTY Uppercase
char kbd_us_uppercase[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,
    0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' '
};

void keyboard_handle() {
    uint8_t scancode = inb(0x60);

    // Handle Shift Press
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        outb(0x20, 0x20);
        return;
    }

    // Handle Shift Release
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = false;
        outb(0x20, 0x20);
        return;
    }

    // If the top bit is set, it's a key release (break code), ignore it
    if (scancode > 0x80) {
        // Send EOI to Master PIC
        outb(0x20, 0x20);
        return;
    }

    char c = 0;
    // Handle Enter key (0x1C)
    if (scancode == 0x1C) {
        c = '\n';
    } 
    // Handle Backspace (0x0E)
    else if (scancode == 0x0E) {
        c = '\b';
    }
    // Handle Escape (0x01)
    else if (scancode == 0x01) {
        c = 0x1B;
    }
    // Printable characters
    else if (scancode < 59) {
        if (shift_pressed) {
            c = kbd_us_uppercase[scancode];
        } else {
            c = kbd_us_lowercase[scancode];
        }
    }

    if (c != 0) {
        uint8_t next = (write_ptr + 1) % 256;
        if (next != read_ptr) {
            buffer[write_ptr] = c;
            write_ptr = next;
        }
    }

    // Send EOI to Master PIC
    outb(0x20, 0x20);
}

char keyboard_get_char() {
    while (read_ptr == write_ptr) {
        asm volatile("hlt");
    }
    
    char c = buffer[read_ptr];
    read_ptr = (read_ptr + 1) % 256;
    return c;
}
