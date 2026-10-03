#include "block.h"
#include "ata.h"

#define BLOCK_SECTOR_SIZE 512UL

static int block_ready = 0;

int block_init(void)
{
    if (block_ready)
        return 0;

    if (ata_init() != 0)
        return -1;

    if (ata_sector_count() == 0)
        return -1;

    block_ready = 1;

    return 0;
}

unsigned long block_sector_count(void)
{
    if (!block_ready)
    {
        if (block_init() != 0)
            return 0;
    }

    return ata_sector_count();
}

int block_read(
    unsigned long lba,
    void *buffer
)
{
    if (buffer == 0)
        return -1;

    if (!block_ready)
    {
        if (block_init() != 0)
            return -1;
    }

    if (lba >= block_sector_count())
        return -1;

    return ata_read_sector(
        lba,
        buffer
    );
}

int block_write(
    unsigned long lba,
    const void *buffer
)
{
    if (buffer == 0)
        return -1;

    if (!block_ready)
    {
        if (block_init() != 0)
            return -1;
    }

    if (lba >= block_sector_count())
        return -1;

    return ata_write_sector(
        lba,
        buffer
    );
}
