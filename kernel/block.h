#ifndef TRIANGLEOS_BLOCK_H
#define TRIANGLEOS_BLOCK_H

int block_init(void);

unsigned long block_sector_count(void);

int block_read(
    unsigned long lba,
    void *buffer
);

int block_write(
    unsigned long lba,
    const void *buffer
);

#endif
