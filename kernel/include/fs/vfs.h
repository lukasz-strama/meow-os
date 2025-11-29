#pragma once
#include <stdint.h>

#define FS_FILE        0x01
#define FS_DIRECTORY   0x02
#define FS_CHARDEVICE  0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE        0x05
#define FS_SYMLINK     0x06
#define FS_MOUNTPOINT  0x08

struct fs_node;

typedef uint32_t (*read_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef uint32_t (*write_type_t)(struct fs_node*, uint32_t, uint32_t, uint8_t*);
typedef void (*open_type_t)(struct fs_node*);
typedef void (*close_type_t)(struct fs_node*);
typedef struct fs_node* (*finddir_type_t)(struct fs_node*, char* name);

typedef struct fs_node {
    char name[32];
    uint32_t flags;
    uint32_t inode;
    uint32_t size;
    uint32_t impl; // Implementation defined data (e.g. cluster number)
    
    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    finddir_type_t finddir;
    
    struct fs_node* ptr; // Used for mount points and symlinks
} fs_node_t;

extern fs_node_t* fs_root; // The root of the filesystem

// Standard VFS functions
uint32_t vfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t vfs_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
void vfs_open(fs_node_t* node);
void vfs_close(fs_node_t* node);
fs_node_t* vfs_finddir(fs_node_t* node, char* name);

// File Descriptor Management
void vfs_init_fds();
int vfs_open_file(char* filename, int flags);
void vfs_close_file(int fd);
int vfs_read_file(int fd, uint8_t* buffer, uint32_t size);
int vfs_write_file(int fd, uint8_t* buffer, uint32_t size);
void vfs_close_all();

