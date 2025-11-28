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

// --- NEW DIRECTORY SUPPORT ---

int fat_find_entry(uint16_t dir_cluster, char* name, FAT_DirectoryEntry* entry_out) {
    char dos_name[11];
    to_dos_filename(name, dos_name);
    
    FAT_DirectoryEntry* buf = (FAT_DirectoryEntry*)malloc(512 * sectors_per_cluster);
    if (!buf) return 0;

    if (dir_cluster == 0) {
        // Root Dir
        uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
        for (int i = 0; i < root_sectors; i++) {
            if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)buf) != 0) break;
            for (int j = 0; j < 16; j++) {
                if (buf[j].filename[0] == 0x00) { free(buf); return 0; }
                if ((uint8_t)buf[j].filename[0] == 0xE5) continue;
                
                int match = 1;
                for (int k=0; k<11; k++) if (buf[j].filename[k] != dos_name[k]) match=0;
                if (match) {
                    *entry_out = buf[j];
                    free(buf);
                    return 1;
                }
            }
        }
    } else {
        // Chain
        uint16_t current = dir_cluster;
        while (current < 0xFFF8) {
            uint32_t lba = data_start_sector + (current - 2) * sectors_per_cluster;
            if (ata_read_sectors(lba, sectors_per_cluster, (uint16_t*)buf) != 0) break;
            
            int count = (512 * sectors_per_cluster) / 32;
            for (int j = 0; j < count; j++) {
                if (buf[j].filename[0] == 0x00) { free(buf); return 0; }
                if ((uint8_t)buf[j].filename[0] == 0xE5) continue;
                
                int match = 1;
                for (int k=0; k<11; k++) if (buf[j].filename[k] != dos_name[k]) match=0;
                if (match) {
                    *entry_out = buf[j];
                    free(buf);
                    return 1;
                }
            }
            current = fat_read_fat_entry(current);
        }
    }
    free(buf);
    return 0;
}

int fat_resolve_path(char* path, uint16_t* cluster_out, uint32_t* size_out, uint8_t* is_dir_out) {
    uint16_t current_cluster = 0; // Root
    
    // Handle root path "/"
    if (path[0] == '/' && path[1] == '\0') {
        if (cluster_out) *cluster_out = 0;
        if (is_dir_out) *is_dir_out = 1;
        return 1;
    }

    char* p = path;
    if (*p == '/') p++; // Skip leading slash
    
    char segment[12];
    while (*p) {
        int i = 0;
        while (*p && *p != '/') {
            if (i < 11) segment[i++] = *p;
            p++;
        }
        segment[i] = '\0';
        if (*p == '/') p++;
        
        if (segment[0] == '\0') continue; // Trailing slash

        FAT_DirectoryEntry entry;
        if (!fat_find_entry(current_cluster, segment, &entry)) return 0;
        
        current_cluster = entry.first_cluster_low;
        if (size_out) *size_out = entry.file_size;
        if (is_dir_out) *is_dir_out = (entry.attributes & FAT_ATTR_DIRECTORY) ? 1 : 0;
        
        if (*p && !(entry.attributes & FAT_ATTR_DIRECTORY)) return 0; // Path continues but not a dir
    }
    
    if (cluster_out) *cluster_out = current_cluster;
    return 1;
}

void fat_ls(char* path) {
    uint16_t cluster;
    uint8_t is_dir;
    
    // Default to root if path is null or empty
    if (!path || path[0] == '\0') path = "/";

    if (!fat_resolve_path(path, &cluster, NULL, &is_dir)) {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        printf("Path not found: %s\n", path);
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        return;
    }
    if (!is_dir) {
        printf("%s is not a directory.\n", path);
        return;
    }

    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512 * sectors_per_cluster);
    if (!dir) return;

    print_set_color(PRINT_COLOR_YELLOW, PRINT_COLOR_BLACK);
    printf("Directory listing for %s:\n", path);
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);

    if (cluster == 0) {
        uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
        for (int i = 0; i < root_sectors; i++) {
            if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) break;
            for (int j = 0; j < 16; j++) {
                FAT_DirectoryEntry* entry = &dir[j];
                if (entry->filename[0] == 0x00) break;
                if ((uint8_t)entry->filename[0] == 0xE5) continue;
                if (entry->attributes == 0x0F) continue;

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
    } else {
        uint16_t current = cluster;
        while (current < 0xFFF8) {
            uint32_t lba = data_start_sector + (current - 2) * sectors_per_cluster;
            if (ata_read_sectors(lba, sectors_per_cluster, (uint16_t*)dir) != 0) break;
            
            int count = (512 * sectors_per_cluster) / 32;
            for (int j = 0; j < count; j++) {
                FAT_DirectoryEntry* entry = &dir[j];
                if (entry->filename[0] == 0x00) break;
                if ((uint8_t)entry->filename[0] == 0xE5) continue;
                if (entry->attributes == 0x0F) continue;

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
            current = fat_read_fat_entry(current);
        }
    }
    free(dir);
}

uint16_t fat_read_fat_entry(uint16_t cluster);

void fat_read_file(char* path) {
    uint16_t cluster;
    uint32_t size;
    uint8_t is_dir;
    
    if (!fat_resolve_path(path, &cluster, &size, &is_dir)) {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        printf("File not found: %s\n", path);
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        return;
    }
    
    if (is_dir) {
        printf("%s is a directory.\n", path);
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

int fat_read_file_to_buffer(char* path, char* buffer, int max_len) {
    uint16_t cluster;
    uint32_t size;
    uint8_t is_dir;
    
    if (!fat_resolve_path(path, &cluster, &size, &is_dir)) return 0;
    if (is_dir) return 0;

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

void fat_create_entry(uint16_t parent_cluster, char* filename, uint8_t attr, uint16_t cluster, uint32_t size) {
    char dos_name[11];
    to_dos_filename(filename, dos_name);

    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512 * sectors_per_cluster);
    if (!dir) return;

    int found = 0;
    uint32_t sector_to_write = 0;
    int entry_index = 0;

    if (parent_cluster == 0) {
        uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
        for (int i = 0; i < root_sectors; i++) {
            if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) break;
            for (int j = 0; j < 16; j++) {
                if (dir[j].filename[0] == 0x00 || (uint8_t)dir[j].filename[0] == 0xE5) {
                    found = 1;
                    sector_to_write = root_start_sector + i;
                    entry_index = j;
                    break;
                }
            }
            if (found) break;
        }
    } else {
        uint16_t current = parent_cluster;
        while (current < 0xFFF8) {
            uint32_t lba = data_start_sector + (current - 2) * sectors_per_cluster;
            if (ata_read_sectors(lba, sectors_per_cluster, (uint16_t*)dir) != 0) break;
            
            int count = (512 * sectors_per_cluster) / 32;
            for (int j = 0; j < count; j++) {
                if (dir[j].filename[0] == 0x00 || (uint8_t)dir[j].filename[0] == 0xE5) {
                    found = 1;
                    sector_to_write = lba;
                    entry_index = j;
                    break;
                }
            }
            if (found) break;
            current = fat_read_fat_entry(current);
        }
    }

    if (found) {
        FAT_DirectoryEntry* entry = &dir[entry_index];
        for (int k = 0; k < 11; k++) entry->filename[k] = dos_name[k];
        entry->attributes = attr;
        entry->reserved = 0;
        entry->creation_time = 0;
        entry->creation_date = 0;
        entry->last_access_date = 0;
        entry->first_cluster_high = 0;
        entry->last_mod_time = 0;
        entry->last_mod_date = 0;
        entry->first_cluster_low = cluster;
        entry->file_size = size;
        
        ata_write_sectors(sector_to_write, (parent_cluster == 0) ? 1 : sectors_per_cluster, (uint16_t*)dir);
    } else {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        print_str("FAT: Directory Full!\n");
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    }
    free(dir);
}

void fat_mkdir(char* path) {
    char parent_path[128];
    char dirname[12];
    
    int len = 0;
    while(path[len]) len++;
    
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') { last_slash = i; break; }
    }
    
    if (last_slash == -1) {
        parent_path[0] = '/'; parent_path[1] = '\0';
        int k=0; for(int i=0; i<len; i++) dirname[k++] = path[i]; dirname[k] = '\0';
    } else {
        int i; for (i = 0; i < last_slash; i++) parent_path[i] = path[i];
        if (last_slash == 0) { parent_path[0] = '/'; parent_path[1] = '\0'; } else parent_path[i] = '\0';
        int k = 0; for (int j = last_slash + 1; j < len; j++) dirname[k++] = path[j]; dirname[k] = '\0';
    }
    
    uint16_t parent_cluster;
    uint8_t is_dir;
    if (!fat_resolve_path(parent_path, &parent_cluster, NULL, &is_dir) || !is_dir) {
        printf("Invalid parent directory: %s\n", parent_path);
        return;
    }
    
    uint16_t new_cluster = fat_find_free_cluster();
    if (new_cluster == 0xFFFF) {
        printf("Disk Full!\n");
        return;
    }
    fat_write_fat_entry(new_cluster, 0xFFFF); // EOF
    
    uint16_t* zero_buf = (uint16_t*)malloc(512 * sectors_per_cluster);
    if (zero_buf) {
        for (int i=0; i<256*sectors_per_cluster; i++) zero_buf[i] = 0;
        uint32_t lba = data_start_sector + (new_cluster - 2) * sectors_per_cluster;
        
        FAT_DirectoryEntry* dot_entries = (FAT_DirectoryEntry*)zero_buf;
        
        FAT_DirectoryEntry* dot = &dot_entries[0];
        for(int i=0; i<11; i++) dot->filename[i] = ' ';
        dot->filename[0] = '.';
        dot->attributes = FAT_ATTR_DIRECTORY;
        dot->first_cluster_low = new_cluster;
        
        FAT_DirectoryEntry* dotdot = &dot_entries[1];
        for(int i=0; i<11; i++) dotdot->filename[i] = ' ';
        dotdot->filename[0] = '.';
        dotdot->filename[1] = '.';
        dotdot->attributes = FAT_ATTR_DIRECTORY;
        dotdot->first_cluster_low = parent_cluster;
        
        ata_write_sectors(lba, sectors_per_cluster, zero_buf);
        free(zero_buf);
    }
    
    fat_create_entry(parent_cluster, dirname, FAT_ATTR_DIRECTORY, new_cluster, 0);
    printf("Directory created: %s\n", path);
}

void fat_create_file(char* path, char* content) {
    char parent_path[128];
    char filename[12];
    
    int len = 0;
    while(path[len]) len++;
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) {
        if (path[i] == '/') { last_slash = i; break; }
    }
    
    if (last_slash == -1) {
        parent_path[0] = '/'; parent_path[1] = '\0';
        int k=0; for(int i=0; i<len; i++) filename[k++] = path[i]; filename[k] = '\0';
    } else {
        int i; for (i = 0; i < last_slash; i++) parent_path[i] = path[i];
        if (last_slash == 0) { parent_path[0] = '/'; parent_path[1] = '\0'; } else parent_path[i] = '\0';
        int k = 0; for (int j = last_slash + 1; j < len; j++) filename[k++] = path[j]; filename[k] = '\0';
    }

    uint16_t parent_cluster;
    uint8_t is_dir;
    if (!fat_resolve_path(parent_path, &parent_cluster, NULL, &is_dir) || !is_dir) {
        printf("Invalid parent directory: %s\n", parent_path);
        return;
    }

    uint16_t cluster = fat_find_free_cluster();
    if (cluster == 0xFFFF) {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        print_str("FAT: Disk Full!\n");
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
        return;
    }

    uint32_t lba = data_start_sector + (cluster - 2) * sectors_per_cluster;
    uint16_t* buffer = (uint16_t*)malloc(512 * sectors_per_cluster);
    if (!buffer) {
        print_str("FAT: Memory allocation failed in create_file!\n");
        return;
    }
    
    uint8_t* byte_buf = (uint8_t*)buffer;
    int cluster_size = 512 * sectors_per_cluster;
    for (int i = 0; i < cluster_size; i++) byte_buf[i] = 0;

    int content_len = 0;
    while (content[content_len] && content_len < cluster_size) {
        byte_buf[content_len] = content[content_len];
        content_len++;
    }

    if (ata_write_sectors(lba, sectors_per_cluster, buffer) != 0) {
        print_str("FAT: Disk write error in create_file!\n");
        free(buffer);
        return;
    }
    free(buffer);

    fat_write_fat_entry(cluster, 0xFFFF); // EOF
    fat_create_entry(parent_cluster, filename, FAT_ATTR_ARCHIVE, cluster, content_len);
    
    print_set_color(PRINT_COLOR_LIGHT_GREEN, PRINT_COLOR_BLACK);
    printf("Created file %s (Cluster %d, Size %d)\n", path, cluster, content_len);
    print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
}

void fat_delete_file(char* path) {
    char parent_path[128];
    char filename[12];
    
    int len = 0; while(path[len]) len++;
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) { if (path[i] == '/') { last_slash = i; break; } }
    
    if (last_slash == -1) {
        parent_path[0] = '/'; parent_path[1] = '\0';
        int k=0; for(int i=0; i<len; i++) filename[k++] = path[i]; filename[k] = '\0';
    } else {
        int i; for (i = 0; i < last_slash; i++) parent_path[i] = path[i];
        if (last_slash == 0) { parent_path[0] = '/'; parent_path[1] = '\0'; } else parent_path[i] = '\0';
        int k = 0; for (int j = last_slash + 1; j < len; j++) filename[k++] = path[j]; filename[k] = '\0';
    }

    uint16_t parent_cluster;
    uint8_t is_dir;
    if (!fat_resolve_path(parent_path, &parent_cluster, NULL, &is_dir) || !is_dir) {
        printf("Invalid parent directory: %s\n", parent_path);
        return;
    }

    char dos_name[11];
    to_dos_filename(filename, dos_name);

    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512 * sectors_per_cluster);
    if (!dir) return;
    
    int found = 0;
    uint32_t sector_to_write = 0;
    uint16_t cluster = 0;

    if (parent_cluster == 0) {
        uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
        for (int i = 0; i < root_sectors; i++) {
            if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) break;
            for (int j = 0; j < 16; j++) {
                if (dir[j].filename[0] == 0x00) break;
                if ((uint8_t)dir[j].filename[0] == 0xE5) continue;
                
                int match = 1;
                for (int k=0; k<11; k++) if (dir[j].filename[k] != dos_name[k]) match=0;
                if (match) {
                    cluster = dir[j].first_cluster_low;
                    dir[j].filename[0] = 0xE5;
                    found = 1;
                    sector_to_write = root_start_sector + i;
                    break;
                }
            }
            if (found) break;
        }
    } else {
        uint16_t current = parent_cluster;
        while (current < 0xFFF8) {
            uint32_t lba = data_start_sector + (current - 2) * sectors_per_cluster;
            if (ata_read_sectors(lba, sectors_per_cluster, (uint16_t*)dir) != 0) break;
            
            int count = (512 * sectors_per_cluster) / 32;
            for (int j = 0; j < count; j++) {
                if (dir[j].filename[0] == 0x00) break;
                if ((uint8_t)dir[j].filename[0] == 0xE5) continue;
                
                int match = 1;
                for (int k=0; k<11; k++) if (dir[j].filename[k] != dos_name[k]) match=0;
                if (match) {
                    cluster = dir[j].first_cluster_low;
                    dir[j].filename[0] = 0xE5;
                    found = 1;
                    sector_to_write = lba;
                    break;
                }
            }
            if (found) break;
            current = fat_read_fat_entry(current);
        }
    }

    if (found) {
        ata_write_sectors(sector_to_write, (parent_cluster == 0) ? 1 : sectors_per_cluster, (uint16_t*)dir);
        fat_free_chain(cluster);
        print_set_color(PRINT_COLOR_LIGHT_GREEN, PRINT_COLOR_BLACK);
        printf("Deleted file %s\n", path);
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    } else {
        print_set_color(PRINT_COLOR_LIGHT_RED, PRINT_COLOR_BLACK);
        printf("File not found: %s\n", path);
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
    }
    free(dir);
}

void fat_rmdir(char* path) {
    uint16_t target_cluster;
    uint8_t is_dir;
    
    if (!fat_resolve_path(path, &target_cluster, NULL, &is_dir)) {
        printf("Directory not found: %s\n", path);
        return;
    }
    
    if (!is_dir) {
        printf("Not a directory: %s\n", path);
        return;
    }
    
    if (target_cluster == 0) {
        printf("Cannot delete root directory.\n");
        return;
    }

    // Check if directory is empty
    FAT_DirectoryEntry* buf = (FAT_DirectoryEntry*)malloc(512 * sectors_per_cluster);
    if (!buf) return;

    int is_empty = 1;
    uint16_t current = target_cluster;
    
    while (current < 0xFFF8 && is_empty) {
        uint32_t lba = data_start_sector + (current - 2) * sectors_per_cluster;
        if (ata_read_sectors(lba, sectors_per_cluster, (uint16_t*)buf) != 0) {
            free(buf);
            return;
        }
        
        int count = (512 * sectors_per_cluster) / 32;
        for (int j = 0; j < count; j++) {
            if (buf[j].filename[0] == 0x00) break;
            if ((uint8_t)buf[j].filename[0] == 0xE5) continue;
            
            // Check for . and ..
            if (buf[j].filename[0] == '.') {
                if (buf[j].filename[1] == ' ' || (buf[j].filename[1] == '.' && buf[j].filename[2] == ' ')) {
                    continue;
                }
            }
            
            is_empty = 0;
            break;
        }
        current = fat_read_fat_entry(current);
    }
    free(buf);

    if (!is_empty) {
        printf("Directory not empty: %s\n", path);
        return;
    }

    // Proceed to delete
    // We need to find the entry in the parent directory to remove it
    char parent_path[128];
    char dirname[12];
    
    int len = 0; while(path[len]) len++;
    int last_slash = -1;
    for (int i = len - 1; i >= 0; i--) { if (path[i] == '/') { last_slash = i; break; } }
    
    if (last_slash == -1) {
        parent_path[0] = '/'; parent_path[1] = '\0';
        int k=0; for(int i=0; i<len; i++) dirname[k++] = path[i]; dirname[k] = '\0';
    } else {
        int i; for (i = 0; i < last_slash; i++) parent_path[i] = path[i];
        if (last_slash == 0) { parent_path[0] = '/'; parent_path[1] = '\0'; } else parent_path[i] = '\0';
        int k = 0; for (int j = last_slash + 1; j < len; j++) dirname[k++] = path[j]; dirname[k] = '\0';
    }

    uint16_t parent_cluster;
    if (!fat_resolve_path(parent_path, &parent_cluster, NULL, &is_dir)) {
        printf("Parent directory not found (unexpected).\n");
        return;
    }

    char dos_name[11];
    to_dos_filename(dirname, dos_name);

    FAT_DirectoryEntry* dir = (FAT_DirectoryEntry*)malloc(512 * sectors_per_cluster);
    if (!dir) return;
    
    int found = 0;
    uint32_t sector_to_write = 0;

    if (parent_cluster == 0) {
        uint32_t root_sectors = ((root_dir_entries * 32) + bytes_per_sector - 1) / bytes_per_sector;
        for (int i = 0; i < root_sectors; i++) {
            if (ata_read_sectors(root_start_sector + i, 1, (uint16_t*)dir) != 0) break;
            for (int j = 0; j < 16; j++) {
                if (dir[j].filename[0] == 0x00) break;
                if ((uint8_t)dir[j].filename[0] == 0xE5) continue;
                
                int match = 1;
                for (int k=0; k<11; k++) if (dir[j].filename[k] != dos_name[k]) match=0;
                if (match) {
                    dir[j].filename[0] = 0xE5; // Mark deleted
                    found = 1;
                    sector_to_write = root_start_sector + i;
                    break;
                }
            }
            if (found) break;
        }
    } else {
        uint16_t current = parent_cluster;
        while (current < 0xFFF8) {
            uint32_t lba = data_start_sector + (current - 2) * sectors_per_cluster;
            if (ata_read_sectors(lba, sectors_per_cluster, (uint16_t*)dir) != 0) break;
            
            int count = (512 * sectors_per_cluster) / 32;
            for (int j = 0; j < count; j++) {
                if (dir[j].filename[0] == 0x00) break;
                if ((uint8_t)dir[j].filename[0] == 0xE5) continue;
                
                int match = 1;
                for (int k=0; k<11; k++) if (dir[j].filename[k] != dos_name[k]) match=0;
                if (match) {
                    dir[j].filename[0] = 0xE5; // Mark deleted
                    found = 1;
                    sector_to_write = lba;
                    break;
                }
            }
            if (found) break;
            current = fat_read_fat_entry(current);
        }
    }

    if (found) {
        ata_write_sectors(sector_to_write, (parent_cluster == 0) ? 1 : sectors_per_cluster, (uint16_t*)dir);
        fat_free_chain(target_cluster);
        printf("Directory deleted: %s\n", path);
    } else {
        printf("Could not find directory entry to delete.\n");
    }
    free(dir);
}
