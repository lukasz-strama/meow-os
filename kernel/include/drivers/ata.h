#pragma once
#include <stdint.h>

#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_SECTOR_CNT 0x1F2
#define ATA_LBA_LO     0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HI     0x1F5
#define ATA_DRIVE_HEAD 0x1F6
#define ATA_STATUS     0x1F7
#define ATA_COMMAND    0x1F7

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7

#define ATA_SR_BSY     0x80    // Busy
#define ATA_SR_DRQ     0x08    // Data Request ready

void ata_read_sectors(uint32_t lba, uint8_t total_sectors, uint16_t* buffer);
void ata_write_sectors(uint32_t lba, uint8_t total_sectors, uint16_t* buffer);
