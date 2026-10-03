#ifndef TRIANGLEOS_ATA_H
#define TRIANGLEOS_ATA_H

int ata_init(void);

unsigned long ata_sector_count(void);

int ata_read_sector(
    unsigned long lba,
    void *buffer
);

int ata_write_sector(
    unsigned long lba,
    const void *buffer
);

#endif
