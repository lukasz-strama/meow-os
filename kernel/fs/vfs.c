#include "fs/vfs.h"
#include "fs/devfs.h"
#include "drivers/print.h"

fs_node_t* fs_root = 0;

// File Descriptor Table
#define MAX_OPEN_FILES 16
struct fs_node* file_descriptors[MAX_OPEN_FILES];
uint32_t file_offsets[MAX_OPEN_FILES]; // Track current read/write offset

void vfs_init_fds() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        file_descriptors[i] = 0;
        file_offsets[i] = 0;
    }
}

int vfs_get_free_fd() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_descriptors[i] == 0) return i;
    }
    return -1;
}

// Helper to check prefix
static int str_starts_with(const char* str, const char* prefix) {
    while (*prefix) {
        if (*prefix++ != *str++) return 0;
    }
    return 1;
}

// Helper to get filename from path (simple version)
static char* get_filename(char* path) {
    char* last_slash = 0;
    char* p = path;
    while (*p) {
        if (*p == '/') last_slash = p;
        p++;
    }
    if (last_slash) return last_slash + 1;
    return path;
}

int vfs_open_file(char* filename, int flags) {
    fs_node_t* node = 0;

    if (str_starts_with(filename, "/dev/")) {
        // Device file
        char* dev_name = filename + 5; // Skip "/dev/"
        if (devfs_root_node) {
            node = vfs_finddir(devfs_root_node, dev_name);
        }
    } else {
        // Regular file (FAT) path traversal
        node = fs_root;
        if (!node) return -1;

        char* p = filename;
        if (*p == '/') p++; // Skip leading slash
        
        char component[128];
        int i = 0;
        
        while (*p) {
            i = 0;
            while (*p && *p != '/') {
                if (i < 127) component[i++] = *p;
                p++;
            }
            component[i] = '\0';
            
            if (i > 0) {
                fs_node_t* next_node = vfs_finddir(node, component);
                
                // If we were holding an intermediate node (not root), free it
                if (node != fs_root) {
                    // TODO: Implement proper node reference counting to avoid double frees.
                    // free(node); 
                }
                
                if (!next_node) return -1; // Path component not found
                node = next_node;
            }
            
            if (*p == '/') p++;
        }
    }

    if (!node) return -1;

    int fd = vfs_get_free_fd();
    if (fd == -1) return -1;

    file_descriptors[fd] = node;
    file_offsets[fd] = 0;
    vfs_open(node);
    
    return fd;
}

void vfs_close_file(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return;
    if (file_descriptors[fd]) {
        vfs_close(file_descriptors[fd]);
        // TODO: Handle node deallocation (check if dynamic or static).
        
        file_descriptors[fd] = 0;
        file_offsets[fd] = 0;
    }
}

void vfs_close_all() {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (file_descriptors[i]) {
            vfs_close_file(i);
        }
    }
}

int vfs_read_file(int fd, uint8_t* buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -1;
    fs_node_t* node = file_descriptors[fd];
    if (!node) return -1;

    uint32_t bytes_read = vfs_read(node, file_offsets[fd], size, buffer);
    file_offsets[fd] += bytes_read;
    return bytes_read;
}

int vfs_write_file(int fd, uint8_t* buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) return -1;
    fs_node_t* node = file_descriptors[fd];
    if (!node) {
        printf("VFS: Write failed, invalid FD %d\n", fd);
        return -1;
    }

    // printf("VFS: Writing to FD %d (Node: %s)\n", fd, node->name);
    uint32_t bytes_written = vfs_write(node, file_offsets[fd], size, buffer);
    file_offsets[fd] += bytes_written;
    return bytes_written;
}

uint32_t vfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (node->read)
        return node->read(node, offset, size, buffer);
    return 0;
}

uint32_t vfs_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    // printf("vfs_write: node=%x, offset=%d, size=%d\n", node, offset, size);
    if (node->write)
        return node->write(node, offset, size, buffer);
    return 0;
}

void vfs_open(fs_node_t* node) {
    if (node->open)
        node->open(node);
}

void vfs_close(fs_node_t* node) {
    if (node->close)
        node->close(node);
}

fs_node_t* vfs_finddir(fs_node_t* node, char* name) {
    if (node->flags & FS_DIRECTORY && node->finddir)
        return node->finddir(node, name);
    return 0;
}
