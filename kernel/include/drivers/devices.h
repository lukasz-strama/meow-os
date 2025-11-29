#ifndef DEVICES_H
#define DEVICES_H

#include <stdint.h>
#include "fs/vfs.h"

uint32_t console_write_fs(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t keyboard_read_fs(struct fs_node* node, uint32_t offset, uint32_t size, uint8_t* buffer);

#endif
