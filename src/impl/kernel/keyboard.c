#include "keyboard.h"
#include "print.h"
#include "io.h"

static char buffer[256];
static uint8_t write_ptr = 0;
static uint8_t read_ptr = 0;

// Simple scancode to ASCII table (incomplete, just basics)
char scancode_to_char[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void keyboard_handle() {
    uint8_t scancode = inb(0x60);

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
    // Printable characters
    else if (scancode < 59) {
        c = scancode_to_char[scancode];
    }

    if (c != 0) {
        uint8_t next = (write_ptr + 1) % 256;
        if (next != read_ptr) {
            buffer[write_ptr] = c;
            write_ptr = next;

            // Echo to screen
            if (c == '\b') {
                print_backspace();
            } else {
                print_char(c);
            }
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
