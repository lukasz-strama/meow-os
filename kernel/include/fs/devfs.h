#ifndef DEVFS_H
#define DEVFS_H

#include "fs/vfs.h"

extern struct fs_node* devfs_root_node;

// Initialize the DevFS root node
struct fs_node* devfs_init();

// Register a device (driver) under a filename
void devfs_register(char* name, 
                    uint32_t (*read)(struct fs_node*, uint32_t, uint32_t, uint8_t*),
                    uint32_t (*write)(struct fs_node*, uint32_t, uint32_t, uint8_t*));

#endif
