#include "keyboard.h"
#include "print.h"
#include "io.h"

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

    // Handle Enter key (0x1C)
    if (scancode == 0x1C) {
        print_char('\n');
    } 
    // Handle Backspace (0x0E)
    else if (scancode == 0x0E) {
        print_backspace();
    }
    // Printable characters
    else if (scancode < 59) {
        char c = scancode_to_char[scancode];
        if (c != 0) {
            print_char(c);
        }
    }

    // Send EOI to Master PIC
    outb(0x20, 0x20);
}
