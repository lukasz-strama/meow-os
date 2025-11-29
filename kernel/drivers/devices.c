#include "drivers/devices.h"
#include "drivers/print.h"
#include "drivers/keyboard.h"

uint32_t console_write_fs(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    for (uint32_t i = 0; i < size; i++) {
        print_char((char)buffer[i]);
    }
    return size;
}

uint32_t keyboard_read_fs(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    for (uint32_t i = 0; i < size; i++) {
        buffer[i] = (uint8_t)keyboard_get_char();
    }
    return size;
}
