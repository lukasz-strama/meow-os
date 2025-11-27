#include "fat.h"
#include "ata.h"
#include "print.h"
#include "heap.h"

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
    ata_read_sectors(0, 1, (uint16_t*)bpb);

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

    print_str("Files:\n");

    for (int i = 0; i < root_sectors; i++) {
        ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir);

        for (int j = 0; j < 16; j++) { // 512 / 32 = 16 entries per sector
            FAT_DirectoryEntry* entry = &dir[j];

            if (entry->filename[0] == 0x00) break; // End of directory
            if (entry->filename[0] == 0xE5) continue; // Deleted
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
                printf("  [DIR] %s\n", name);
            } else {
                printf("  %s (%d bytes)\n", name, entry->file_size);
            }
        }
    }
    free(dir);
}

void fat_read_file(char* filename) {
    char dos_name[11];
    to_dos_filename(filename, dos_name);

    uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512);
    
    int found = 0;
    uint16_t cluster = 0;
    uint32_t size = 0;

    for (int i = 0; i < root_sectors; i++) {
        ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir);

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
        printf("File not found: %s\n", filename);
        return;
    }

    // Read file content
    // LBA = data_start + (cluster - 2) * sectors_per_cluster
    uint32_t lba = data_start_sector + (cluster - 2) * sectors_per_cluster;
    
    uint16_t* buffer = (uint16_t*)malloc(512 * sectors_per_cluster);
    ata_read_sectors(lba, sectors_per_cluster, buffer);

    char* text = (char*)buffer;
    for (uint32_t i = 0; i < size; i++) {
        print_char(text[i]);
    }
    print_char('\n');

    free(buffer);
}

void fat_write_fat_entry(uint16_t cluster, uint16_t value) {
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = fat_start_sector + (fat_offset / bytes_per_sector);
    uint32_t ent_offset = fat_offset % bytes_per_sector;

    uint16_t* buffer = (uint16_t*)malloc(512);
    ata_read_sectors(fat_sector, 1, buffer);
    
    buffer[ent_offset / 2] = value;

    ata_write_sectors(fat_sector, 1, buffer);
    free(buffer);
}

uint16_t fat_find_free_cluster() {
    uint16_t* buffer = (uint16_t*)malloc(512);
    
    for (int i = 0; i < sectors_per_fat; i++) {
        ata_read_sectors(fat_start_sector + i, 1, buffer);
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
    
    int found = 0;
    uint32_t sector_to_write = 0;

    for (int i = 0; i < root_sectors; i++) {
        ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir);

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
        print_str("FAT: Root Directory Full!\n");
    }
    free(dir);
}

void fat_create_file(char* filename, char* content) {
    uint16_t cluster = fat_find_free_cluster();
    if (cluster == 0xFFFF) {
        print_str("FAT: Disk Full!\n");
        return;
    }

    // Write Content
    uint32_t lba = data_start_sector + (cluster - 2) * sectors_per_cluster;
    uint16_t* buffer = (uint16_t*)malloc(512 * sectors_per_cluster);
    
    // Clear buffer
    uint8_t* byte_buf = (uint8_t*)buffer;
    for (int i = 0; i < 512 * sectors_per_cluster; i++) byte_buf[i] = 0;

    // Copy content
    int len = 0;
    while (content[len]) {
        byte_buf[len] = content[len];
        len++;
    }

    ata_write_sectors(lba, sectors_per_cluster, buffer);
    free(buffer);

    // Update FAT
    fat_write_fat_entry(cluster, 0xFFFF); // EOF

    // Update Root Dir
    fat_create_root_entry(filename, cluster, len);
    
    printf("Created file %s (Cluster %d, Size %d)\n", filename, cluster, len);
}
