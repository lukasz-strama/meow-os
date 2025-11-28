#include "fs/fat.h"
#include "fs/vfs.h"
#include "drivers/ata.h"
#include "memory/heap.h"
#include "drivers/print.h"

// Forward declarations
uint32_t fat_read_vfs(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer);
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
fs_node_t* fat_entry_to_node(FAT_DirectoryEntry* entry) {
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
    node->write = 0; // Read-only for now via VFS
    node->open = 0;
    node->close = 0;
    node->finddir = fat_finddir_vfs;
    node->ptr = 0;

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

fs_node_t* fat_finddir_vfs(fs_node_t* node, char* name) {
    if (!(node->flags & FS_DIRECTORY)) return 0;

    // TODO: Support subdirectories. For now, only Root Directory is fully supported by fat.c logic structure
    // But we can adapt.
    
    // If node is root (inode 0 or special), we read root dir sectors.
    // If node is subdir, we read its cluster chain like a file.
    
    // Current fat.c logic relies on root_start_sector for root.
    // Let's assume node->inode == 0 means root.
    
    FAT_DirectoryEntry* dir_buf = (FAT_DirectoryEntry*)malloc(512);
    if (!dir_buf) return 0;

    char dos_name[11];
    to_dos_filename(name, dos_name);

    if (node->inode == 0) {
        // Root Directory
        uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
        
        for (int i = 0; i < root_sectors; i++) {
            if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir_buf) != 0) {
                free(dir_buf);
                return 0;
            }

            for (int j = 0; j < 16; j++) {
                FAT_DirectoryEntry* entry = &dir_buf[j];
                if (entry->filename[0] == 0x00) break;
                if (entry->filename[0] == 0xE5) continue;
                if (entry->attributes == 0x0F) continue;

                int match = 1;
                for (int k = 0; k < 11; k++) {
                    if (entry->filename[k] != dos_name[k]) {
                        match = 0;
                        break;
                    }
                }

                if (match) {
                    fs_node_t* found_node = fat_entry_to_node(entry);
                    free(dir_buf);
                    return found_node;
                }
            }
        }
    } else {
        // Subdirectory (read as file)
        // TODO: Implement subdirectory reading
        // It's similar to read_vfs but we interpret content as DirectoryEntries
    }

    free(dir_buf);
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
