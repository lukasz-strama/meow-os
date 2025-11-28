#include "fs/fat.h"
#include "drivers/ata.h"
#include "drivers/print.h"
#include "memory/heap.h"

// Global variables
uint32_t fat_start_sector;
uint32_t root_start_sector;
uint32_t data_start_sector;
uint16_t sectors_per_cluster;
uint16_t bytes_per_sector;
uint16_t root_dir_entries;
uint16_t sectors_per_fat;

// Helper to convert "test.txt" to "TEST    TXT"
void to_dos_filename(const char* input, char* output) {
    // Initialize output with spaces
    for (int i = 0; i < 11; i++) output[i] = ' ';

    int i = 0;
    int j = 0;
    
    // Copy filename part
    while (input[i] != '\0' && input[i] != '.') {
        if (j < 8) {
            char c = input[i];
            if (c >= 'a' && c <= 'z') c -= 32; // To Upper
            output[j] = c;
            j++;
        }
        i++;
    }

    // Skip dot
    if (input[i] == '.') {
        i++;
        j = 8; // Move to extension part
        while (input[i] != '\0') {
            if (j < 11) {
                char c = input[i];
                if (c >= 'a' && c <= 'z') c -= 32; // To Upper
                output[j] = c;
                j++;
            }
            i++;
        }
    }
}

void fat_init() {
    FAT_BPB* bpb = (FAT_BPB*)malloc(512);
    if (!bpb) {
        print_str("FAT: Memory allocation failed during init!\n");
        return;
    }

    if (ata_read_sectors(0, 1, (uint16_t*)bpb) != 0) {
        print_str("FAT: Failed to read Boot Sector!\n");
        free(bpb);
        return;
    }

    // Check the standard signature at the end of the sector
    uint8_t* raw = (uint8_t*)bpb;
    if (raw[510] != 0x55 || raw[511] != 0xAA) {
        print_str("FAT: Invalid Boot Sector Signature!\n");
        free(bpb);
        return;
    }

    sectors_per_cluster = bpb->sectors_per_cluster;
    bytes_per_sector = bpb->bytes_per_sector;
    root_dir_entries = bpb->root_dir_entries;
    sectors_per_fat = bpb->sectors_per_fat;

    fat_start_sector = bpb->reserved_sectors;
    root_start_sector = fat_start_sector + (bpb->fat_count * bpb->sectors_per_fat);
    data_start_sector = root_start_sector + ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;

    printf("FAT16 Initialized.\n");
    printf("  Root Start: %d\n", root_start_sector);
    printf("  Data Start: %d\n", data_start_sector);
    
    free(bpb);
}

void fat_ls() {
    uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512);
    if (!dir) {
        print_str("FAT: Memory allocation failed in ls!\n");
        return;
    }

    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    print_str("Files:\n");
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);

    for (int i = 0; i < root_sectors; i++) {
        if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) {
            print_str("FAT: Disk read error in ls!\n");
            free(dir);
            return;
        }

        for (int j = 0; j < 16; j++) { // 512 / 32 = 16 entries per sector
            FAT_DirectoryEntry* entry = &dir[j];

            if (entry->filename[0] == 0x00) break; // End of directory
            if ((uint8_t)entry->filename[0] == 0xE5) continue; // Deleted
            if (entry->attributes == 0x0F) continue; // LFN

            // Print Filename
            char name[12];
            int k = 0;
            for (int l = 0; l < 8; l++) {
                if (entry->filename[l] != ' ') name[k++] = entry->filename[l];
            }
            if (entry->ext[0] != ' ') {
                name[k++] = '.';
                for (int l = 0; l < 3; l++) {
                    if (entry->ext[l] != ' ') name[k++] = entry->ext[l];
                }
            }
            name[k] = '\0';

            if (entry->attributes & FAT_ATTR_DIRECTORY) {
                print_set_color(PRINT_COLOR_LIGHT_BLUE, PRINT_COLOR_BLACK);
                printf("  [DIR] %s\n", name);
                print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
            } else {
                printf("  %s (%d bytes)\n", name, entry->file_size);
            }
        }
    }
    free(dir);
}

uint16_t fat_read_fat_entry(uint16_t cluster);

void fat_read_file(char* filename) {
    char dos_name[11];
    to_dos_filename(filename, dos_name);

    uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512);
    if (!dir) {
        print_str("FAT: Memory allocation failed in read_file!\n");
        return;
    }
    
    int found = 0;
    uint16_t cluster = 0;
    uint32_t size = 0;

    for (int i = 0; i < root_sectors; i++) {
        if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) {
            print_str("FAT: Disk read error in read_file!\n");
            free(dir);
            return;
        }

        for (int j = 0; j < 16; j++) {
            FAT_DirectoryEntry* entry = &dir[j];

            if (entry->filename[0] == 0x00) break;
            if (entry->filename[0] == 0xE5) continue;
            if (entry->attributes == 0x0F) continue;
            if (entry->attributes & FAT_ATTR_DIRECTORY) continue;

            // Compare filename
            int match = 1;
            for (int k = 0; k < 11; k++) {
                if (entry->filename[k] != dos_name[k]) {
                    match = 0;
                    break;
                }
            }

            if (match) {
                cluster = entry->first_cluster_low;
                size = entry->file_size;
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    free(dir);

    if (!found) {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        printf("File not found: %s\n", filename);
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        return;
    }

    // Read file content
    uint16_t* buffer = (uint16_t*)malloc(512 * sectors_per_cluster);
    if (!buffer) {
        print_str("FAT: Memory allocation failed for file buffer!\n");
        return;
    }

    uint16_t current_cluster = cluster;
    int bytes_read = 0;

    while (bytes_read < size && current_cluster < 0xFFF8) {
        uint32_t lba = data_start_sector + (current_cluster - 2) * sectors_per_cluster;
        if (ata_read_sectors(lba, sectors_per_cluster, buffer) != 0) {
            print_str("FAT: Disk read error during file read!\n");
            free(buffer);
            return;
        }

        char* text = (char*)buffer;
        int cluster_size = 512 * sectors_per_cluster;

        for (int k = 0; k < cluster_size; k++) {
            if (bytes_read < size) {
                print_char(text[k]);
                bytes_read++;
            }
        }
        current_cluster = fat_read_fat_entry(current_cluster);
    }
    print_char('\n');

    free(buffer);
}

int fat_read_file_to_buffer(char* filename, char* buffer, int max_len) {
    char dos_name[11];
    to_dos_filename(filename, dos_name);

    uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512);
    if (!dir) return 0;
    
    int found = 0;
    uint16_t cluster = 0;
    uint32_t size = 0;

    for (int i = 0; i < root_sectors; i++) {
        if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) {
            free(dir);
            return 0;
        }

        for (int j = 0; j < 16; j++) {
            FAT_DirectoryEntry* entry = &dir[j];

            if (entry->filename[0] == 0x00) break;
            if (entry->filename[0] == 0xE5) continue;
            if (entry->attributes == 0x0F) continue;
            if (entry->attributes & FAT_ATTR_DIRECTORY) continue;

            int match = 1;
            for (int k = 0; k < 11; k++) {
                if (entry->filename[k] != dos_name[k]) {
                    match = 0;
                    break;
                }
            }

            if (match) {
                cluster = entry->first_cluster_low;
                size = entry->file_size;
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    free(dir);

    if (!found) {
        return 0; // Not found
    }

    uint16_t* disk_buf = (uint16_t*)malloc(512 * sectors_per_cluster);
    if (!disk_buf) return 0;

    uint16_t current_cluster = cluster;
    int bytes_read = 0;

    while (bytes_read < size && current_cluster < 0xFFF8) {
        uint32_t lba = data_start_sector + (current_cluster - 2) * sectors_per_cluster;
        if (ata_read_sectors(lba, sectors_per_cluster, disk_buf) != 0) {
            free(disk_buf);
            return 0;
        }
        
        char* cluster_data = (char*)disk_buf;
        int cluster_size = 512 * sectors_per_cluster;

        for (int k = 0; k < cluster_size; k++) {
            if (bytes_read < size && bytes_read < max_len) {
                buffer[bytes_read++] = cluster_data[k];
            }
        }
        
        current_cluster = fat_read_fat_entry(current_cluster);
    }

    free(disk_buf);
    return 1; // Success
}

void fat_write_fat_entry(uint16_t cluster, uint16_t value) {
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = fat_start_sector + (fat_offset / bytes_per_sector);
    uint32_t ent_offset = fat_offset % bytes_per_sector;

    uint16_t* buffer = (uint16_t*)malloc(512);
    if (!buffer) return;

    if (ata_read_sectors(fat_sector, 1, buffer) != 0) {
        free(buffer);
        return;
    }
    
    buffer[ent_offset / 2] = value;

    ata_write_sectors(fat_sector, 1, buffer);
    free(buffer);
}

uint16_t fat_read_fat_entry(uint16_t cluster) {
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = fat_start_sector + (fat_offset / bytes_per_sector);
    uint32_t ent_offset = fat_offset % bytes_per_sector;

    uint16_t* buffer = (uint16_t*)malloc(512);
    if (!buffer) return 0xFFFF; // Error

    if (ata_read_sectors(fat_sector, 1, buffer) != 0) {
        free(buffer);
        return 0xFFFF;
    }
    
    uint16_t value = buffer[ent_offset / 2];
    free(buffer);
    return value;
}

void fat_free_chain(uint16_t start_cluster) {
    uint16_t cluster = start_cluster;
    while (cluster < 0xFFF8) {
        uint16_t next = fat_read_fat_entry(cluster);
        if (next == 0xFFFF) break; // Error reading FAT
        fat_write_fat_entry(cluster, 0x0000);
        cluster = next;
    }
}

uint16_t fat_find_free_cluster() {
    uint16_t* buffer = (uint16_t*)malloc(512);
    if (!buffer) return 0xFFFF;
    
    for (int i = 0; i < sectors_per_fat; i++) {
        if (ata_read_sectors(fat_start_sector + i, 1, buffer) != 0) {
            free(buffer);
            return 0xFFFF;
        }
        for (int j = 0; j < 256; j++) {
            if (buffer[j] == 0x0000) {
                uint16_t cluster = (i * 256) + j;
                if (cluster >= 2) { // Clusters 0 and 1 are reserved
                    free(buffer);
                    return cluster;
                }
            }
        }
    }
    free(buffer);
    return 0xFFFF; // Full
}

void fat_create_root_entry(char* filename, uint16_t cluster, uint32_t size) {
    char dos_name[11];
    to_dos_filename(filename, dos_name);

    uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512);
    if (!dir) {
        print_str("FAT: Memory allocation failed in create_root_entry!\n");
        return;
    }
    
    int found = 0;
    uint32_t sector_to_write = 0;

    for (int i = 0; i < root_sectors; i++) {
        if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) {
            print_str("FAT: Disk read error in create_root_entry!\n");
            free(dir);
            return;
        }

        for (int j = 0; j < 16; j++) {
            FAT_DirectoryEntry* entry = &dir[j];

            if (entry->filename[0] == 0x00 || entry->filename[0] == 0xE5) {
                // Found free slot
                for (int k = 0; k < 11; k++) entry->filename[k] = dos_name[k];
                entry->attributes = 0x20; // Archive
                entry->reserved = 0;
                entry->creation_time = 0;
                entry->creation_date = 0;
                entry->last_access_date = 0;
                entry->first_cluster_high = 0;
                entry->last_mod_time = 0;
                entry->last_mod_date = 0;
                entry->first_cluster_low = cluster;
                entry->file_size = size;
                
                found = 1;
                sector_to_write = root_start_sector + i;
                break;
            }
        }
        if (found) break;
    }

    if (found) {
        ata_write_sectors(sector_to_write, 1, (uint16_t*)dir);
    } else {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        print_str("FAT: Root Directory Full!\n");
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    }
    free(dir);
}

void fat_create_file(char* filename, char* content) {
    uint16_t cluster = fat_find_free_cluster();
    if (cluster == 0xFFFF) {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        print_str("FAT: Disk Full!\n");
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        return;
    }

    // Write Content
    uint32_t lba = data_start_sector + (cluster - 2) * sectors_per_cluster;
    uint16_t* buffer = (uint16_t*)malloc(512 * sectors_per_cluster);
    if (!buffer) {
        print_str("FAT: Memory allocation failed in create_file!\n");
        return;
    }
    
    // Clear buffer
    uint8_t* byte_buf = (uint8_t*)buffer;
    int cluster_size = 512 * sectors_per_cluster;
    for (int i = 0; i < cluster_size; i++) byte_buf[i] = 0;

    // Copy content (with overflow protection)
    int len = 0;
    while (content[len] && len < cluster_size) {
        byte_buf[len] = content[len];
        len++;
    }

    if (ata_write_sectors(lba, sectors_per_cluster, buffer) != 0) {
        print_str("FAT: Disk write error in create_file!\n");
        free(buffer);
        return;
    }
    free(buffer);

    // Update FAT
    fat_write_fat_entry(cluster, 0xFFFF); // EOF

    // Update Root Dir
    fat_create_root_entry(filename, cluster, len);
    
    print_set_color(PRINT_COLOR_LIGHT_GREEN, PRINT_COLOR_BLACK);
    printf("Created file %s (Cluster %d, Size %d)\n", filename, cluster, len);
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
}

void fat_delete_file(char* filename) {
    char dos_name[11];
    to_dos_filename(filename, dos_name);

    uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512);
    if (!dir) {
        print_str("FAT: Memory allocation failed in delete_file!\n");
        return;
    }
    
    int found = 0;
    uint32_t sector_to_write = 0;
    uint16_t cluster = 0;

    for (int i = 0; i < root_sectors; i++) {
        if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) {
            print_str("FAT: Disk read error in delete_file!\n");
            free(dir);
            return;
        }

        for (int j = 0; j < 16; j++) {
            FAT_DirectoryEntry* entry = &dir[j];

            if (entry->filename[0] == 0x00) break;
            if (entry->filename[0] == 0xE5) continue;
            if (entry->attributes == 0x0F) continue;
            if (entry->attributes & FAT_ATTR_DIRECTORY) continue;

            // Compare filename
            int match = 1;
            for (int k = 0; k < 11; k++) {
                if (entry->filename[k] != dos_name[k]) {
                    match = 0;
                    break;
                }
            }

            if (match) {
                cluster = entry->first_cluster_low;
                entry->filename[0] = 0xE5; // Mark as deleted
                found = 1;
                sector_to_write = root_start_sector + i;
                break;
            }
        }
        if (found) break;
    }

    if (found) {
        ata_write_sectors(sector_to_write, 1, (uint16_t*)dir);
        fat_free_chain(cluster);
        print_set_color(PRINT_COLOR_LIGHT_GREEN, PRINT_COLOR_BLACK);
        printf("Deleted file %s\n", filename);
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    } else {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        printf("File not found: %s\n", filename);
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    }
    free(dir);
}
