#include "fs/fat.h"
#include "fs/vfs.h"
#include "drivers/ata.h"
#include "memory/heap.h"
#include "drivers/print.h"

// Forward declarations
uint32_t fat_read_vfs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
uint32_t fat_write_vfs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
fs_node_t* fat_finddir_vfs(fs_node_t* node, char* name);

// Helper to compare filenames
int k_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Helper to create a new fs_node from a FAT entry
fs_node_t* fat_entry_to_node(FAT_DirectoryEntry* entry, uint16_t parent_cluster) {
    fs_node_t* node = (fs_node_t*)malloc(sizeof(fs_node_t));
    if (!node) return 0;

    // Set name
    int k = 0;
    for (int l = 0; l < 8; l++) {
        if (entry->filename[l] != ' ') node->name[k++] = entry->filename[l];
    }
    if (entry->ext[0] != ' ') {
        node->name[k++] = '.';
        for (int l = 0; l < 3; l++) {
            if (entry->ext[l] != ' ') node->name[k++] = entry->ext[l];
        }
    }
    node->name[k] = '\0';

    // Set flags
    node->flags = FS_FILE;
    if (entry->attributes & FAT_ATTR_DIRECTORY) {
        node->flags = FS_DIRECTORY;
    }

    node->inode = entry->first_cluster_low; // Use start cluster as inode
    node->size = entry->file_size;
    node->impl = entry->first_cluster_low; // Store start cluster
    
    node->read = fat_read_vfs;
    node->write = fat_write_vfs;
    node->open = 0;
    node->close = 0;
    node->finddir = fat_finddir_vfs;
    node->ptr = (void*)(uintptr_t)parent_cluster;

    return node;
}

uint32_t fat_read_vfs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    if (offset > node->size) return 0;
    if (offset + size > node->size) size = node->size - offset;

    uint16_t current_cluster = node->impl;
    uint32_t cluster_size = sectors_per_cluster * 512;
    
    // Skip clusters to reach offset
    uint32_t clusters_to_skip = offset / cluster_size;
    uint32_t offset_in_cluster = offset % cluster_size;

    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        current_cluster = fat_read_fat_entry(current_cluster);
        if (current_cluster >= 0xFFF8) return 0; // Error or EOF
    }

    // Read data
    uint32_t bytes_read = 0;
    uint8_t* cluster_buffer = (uint8_t*)malloc(cluster_size);
    if (!cluster_buffer) return 0;

    while (bytes_read < size && current_cluster < 0xFFF8) {
        uint32_t lba = data_start_sector + (current_cluster - 2) * sectors_per_cluster;
        if (ata_read_sectors(lba, sectors_per_cluster, (uint16_t*)cluster_buffer) != 0) {
            free(cluster_buffer);
            return bytes_read;
        }

        uint32_t to_copy = cluster_size - offset_in_cluster;
        if (to_copy > size - bytes_read) to_copy = size - bytes_read;

        for (uint32_t i = 0; i < to_copy; i++) {
            buffer[bytes_read + i] = cluster_buffer[offset_in_cluster + i];
        }

        bytes_read += to_copy;
        offset_in_cluster = 0; // Only first cluster has offset
        current_cluster = fat_read_fat_entry(current_cluster);
    }

    free(cluster_buffer);
    return bytes_read;
}

uint32_t fat_write_vfs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer) {
    // printf("FAT Write: %s, off=%d, sz=%d\n", node->name, offset, size);
    uint16_t current_cluster = node->impl;
    uint32_t cluster_size = sectors_per_cluster * 512;
    
    // Skip clusters to reach offset
    uint32_t clusters_to_skip = offset / cluster_size;
    uint32_t offset_in_cluster = offset % cluster_size;

    // Traverse to the start cluster for writing
    for (uint32_t i = 0; i < clusters_to_skip; i++) {
        uint16_t next = fat_read_fat_entry(current_cluster);
        if (next >= 0xFFF8) {
            // Need to allocate new cluster
            uint16_t new_cluster = fat_find_free_cluster();
            if (new_cluster == 0xFFFF) return 0; // Disk full
            fat_write_fat_entry(current_cluster, new_cluster);
            fat_write_fat_entry(new_cluster, 0xFFFF);
            current_cluster = new_cluster;
        } else {
            current_cluster = next;
        }
    }

    uint32_t bytes_written = 0;
    uint8_t* cluster_buffer = (uint8_t*)malloc(cluster_size);
    if (!cluster_buffer) return 0;

    while (bytes_written < size) {
        // Read current cluster to preserve existing data (if partial write)
        uint32_t lba = data_start_sector + (current_cluster - 2) * sectors_per_cluster;
        ata_read_sectors(lba, sectors_per_cluster, (uint16_t*)cluster_buffer);

        uint32_t to_copy = cluster_size - offset_in_cluster;
        if (to_copy > size - bytes_written) to_copy = size - bytes_written;

        for (uint32_t i = 0; i < to_copy; i++) {
            cluster_buffer[offset_in_cluster + i] = buffer[bytes_written + i];
        }

        ata_write_sectors(lba, sectors_per_cluster, (uint16_t*)cluster_buffer);

        bytes_written += to_copy;
        offset_in_cluster = 0;

        if (bytes_written < size) {
            // Need next cluster
            uint16_t next = fat_read_fat_entry(current_cluster);
            if (next >= 0xFFF8) {
                uint16_t new_cluster = fat_find_free_cluster();
                if (new_cluster == 0xFFFF) break; // Disk full
                fat_write_fat_entry(current_cluster, new_cluster);
                fat_write_fat_entry(new_cluster, 0xFFFF);
                current_cluster = new_cluster;
            } else {
                current_cluster = next;
            }
        }
    }
    free(cluster_buffer);

    // Update size if we extended the file
    if (offset + bytes_written > node->size) {
        node->size = offset + bytes_written;
        // printf("FAT Update Size: %s -> %d\n", node->name, node->size);
        fat_update_entry_size((uint16_t)(uintptr_t)node->ptr, node->name, node->size);
    }

    return bytes_written;
}

fs_node_t* fat_finddir_vfs(fs_node_t* node, char* name) {
    if (!(node->flags & FS_DIRECTORY)) return 0;

    FAT_DirectoryEntry entry;
    // node->inode holds the starting cluster of the directory.
    // For root, it is 0.
    if (fat_find_entry(node->inode, name, &entry)) {
        return fat_entry_to_node(&entry, node->inode);
    }

    return 0;
}

void fat_mount() {
    fs_root = (fs_node_t*)malloc(sizeof(fs_node_t));
    if (!fs_root) return;

    k_strcmp(fs_root->name, "root"); // Just to use the function to avoid warning if unused? No, copy name.
    // Manual strcpy
    fs_root->name[0] = '/';
    fs_root->name[1] = '\0';

    fs_root->flags = FS_DIRECTORY;
    fs_root->inode = 0; // 0 for root
    fs_root->read = 0;
    fs_root->write = 0;
    fs_root->open = 0;
    fs_root->close = 0;
    fs_root->finddir = fat_finddir_vfs;
    fs_root->ptr = 0;
    
    printf("VFS: FAT16 mounted at /\n");
}
