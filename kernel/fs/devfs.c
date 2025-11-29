#include "fs/devfs.h"
#include "memory/heap.h"
#include "drivers/print.h"

#define MAX_DEV_NODES 10

static struct fs_node dev_nodes[MAX_DEV_NODES];
static int dev_count = 0;
struct fs_node* devfs_root_node = 0;

// Helper string functions
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static void strcpy(char* dest, const char* src) {
    while ((*dest++ = *src++));
}

static struct fs_node* devfs_finddir(struct fs_node* node, char* name) {
    for (int i = 0; i < dev_count; i++) {
        if (strcmp(name, dev_nodes[i].name) == 0) {
            return &dev_nodes[i];
        }
    }
    return 0;
}

struct fs_node* devfs_init() {
    devfs_root_node = (struct fs_node*)malloc(sizeof(struct fs_node));
    strcpy(devfs_root_node->name, "dev");
    devfs_root_node->flags = FS_DIRECTORY;
    devfs_root_node->finddir = devfs_finddir;
    devfs_root_node->ptr = 0;
    devfs_root_node->inode = 0;
    
    return devfs_root_node;
}

void devfs_register(char* name, 
                    uint32_t (*read)(struct fs_node*, uint32_t, uint32_t, uint8_t*),
                    uint32_t (*write)(struct fs_node*, uint32_t, uint32_t, uint8_t*)) {
    if (dev_count >= MAX_DEV_NODES) {
        printf("DevFS: Max devices reached!\n");
        return;
    }

    struct fs_node* node = &dev_nodes[dev_count];
    strcpy(node->name, name);
    node->flags = FS_CHARDEVICE;
    node->read = read;
    node->write = write;
    node->inode = dev_count;
    
    dev_count++;
    printf("DevFS: Registered device '%s'\n", name);
}
