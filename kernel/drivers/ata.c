#include "drivers/ata.h"
#include "drivers/io.h"
#include "drivers/print.h"

int ata_wait_bsy() {
    // Check for Floating Bus (No Drive)
    if (inb(ATA_STATUS) == 0xFF) {
        print_str("ATA: No Drive (Floating Bus)\n");
        return 1;
    }

    // Wait with timeout
    for (int i = 0; i < 100000; i++) {
        if (!(inb(ATA_STATUS) & ATA_SR_BSY)) {
            return 0;
        }
    }
    
    print_str("ATA: Timeout (BSY)\n");
    return 1;
}

int ata_wait_drq() {
    for (int i = 0; i < 100000; i++) {
        if (inb(ATA_STATUS) & ATA_SR_DRQ) {
            return 0;
        }
    }
    
    print_str("ATA: Timeout (DRQ)\n");
    return 1;
}

int ata_read_sectors(uint32_t lba, uint8_t total_sectors, uint16_t* buffer) {
    if (ata_wait_bsy() != 0) return 1;

    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, total_sectors);
    outb(ATA_LBA_LO, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_READ_PIO);

    for (int i = 0; i < total_sectors; i++) {
        if (ata_wait_bsy() != 0) return 2;
        if (ata_wait_drq() != 0) return 3;

        for (int j = 0; j < 256; j++) {
            buffer[j] = inw(ATA_DATA);
        }
        buffer += 256;
    }
    return 0;
}

int ata_write_sectors(uint32_t lba, uint8_t total_sectors, uint16_t* buffer) {
    if (ata_wait_bsy() != 0) return 1;

    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, total_sectors);
    outb(ATA_LBA_LO, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);

    for (int i = 0; i < total_sectors; i++) {
        if (ata_wait_bsy() != 0) return 2;
        if (ata_wait_drq() != 0) return 3;

        for (int j = 0; j < 256; j++) {
            outw(ATA_DATA, buffer[j]);
        }
        buffer += 256;
    }

    outb(ATA_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (ata_wait_bsy() != 0) return 4;
    
    return 0;
}
