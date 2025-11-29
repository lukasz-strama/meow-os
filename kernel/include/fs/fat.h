#pragma once
#include <stdint.h>

// Boot Sector (BPB)
typedef struct {
    uint8_t jump[3];
    char oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_dir_entries;
    uint16_t total_sectors_16;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed)) FAT_BPB;

// Directory Entry
typedef struct {
    char filename[8];
    char ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) FAT_DirectoryEntry;

#define FAT_ATTR_READ_ONLY 0x01
#define FAT_ATTR_HIDDEN    0x02
#define FAT_ATTR_SYSTEM    0x04
#define FAT_ATTR_VOLUME_ID 0x08
#define FAT_ATTR_DIRECTORY 0x10
#define FAT_ATTR_ARCHIVE   0x20
#define FAT_ATTR_LFN       0x0F

// Globals exposed for VFS adapter
extern uint32_t fat_start_sector;
extern uint32_t root_start_sector;
extern uint32_t data_start_sector;
extern uint16_t sectors_per_cluster;
extern uint16_t bytes_per_sector;
extern uint16_t root_dir_entries;
extern uint16_t sectors_per_fat;

uint16_t fat_read_fat_entry(uint16_t cluster);
void to_dos_filename(const char* input, char* output);

void fat_init();
void fat_mount();
void fat_ls(char* path);
void fat_mkdir(char* path);
void fat_rmdir(char* path);
int fat_resolve_path(char* path, uint16_t* cluster_out, uint32_t* size_out, uint8_t* is_dir_out);
void fat_read_file(char* path);
int fat_read_file_to_buffer(char* path, char* buffer, int max_len);
void fat_create_file(char* path, char* content);
void fat_delete_file(char* path);

int fat_find_entry(uint16_t dir_cluster, char* name, FAT_DirectoryEntry* entry_out);
