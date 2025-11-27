#include "ata.h"
#include "io.h"

void ata_wait_bsy() {
    while (inb(ATA_STATUS) & ATA_SR_BSY);
}

void ata_wait_drq() {
    while (!(inb(ATA_STATUS) & ATA_SR_DRQ));
}

void ata_read_sectors(uint32_t lba, uint8_t total_sectors, uint16_t* buffer) {
    ata_wait_bsy();

    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, total_sectors);
    outb(ATA_LBA_LO, (uint8_t)lba);
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_READ_PIO);

    for (int i = 0; i < total_sectors; i++) {
        ata_wait_bsy();
        ata_wait_drq();

        for (int j = 0; j < 256; j++) {
            buffer[j] = inw(ATA_DATA);
        }
        buffer += 256;
    }
}
