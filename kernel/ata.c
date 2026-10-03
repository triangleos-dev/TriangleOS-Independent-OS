#include "ata.h"
#include "console.h"

#define ATA_DATA          0x1F0
#define ATA_ERROR         0x1F1
#define ATA_SECTOR_COUNT  0x1F2
#define ATA_LBA_LOW       0x1F3
#define ATA_LBA_MID       0x1F4
#define ATA_LBA_HIGH      0x1F5
#define ATA_DRIVE         0x1F6
#define ATA_STATUS        0x1F7
#define ATA_COMMAND       0x1F7
#define ATA_CONTROL       0x3F6

#define ATA_CMD_READ_PIO   0x20
#define ATA_CMD_WRITE_PIO  0x30
#define ATA_CMD_IDENTIFY   0xEC
#define ATA_CMD_CACHE_FLUSH 0xE7

#define ATA_SR_ERR  0x01
#define ATA_SR_DRQ  0x08
#define ATA_SR_DF   0x20
#define ATA_SR_RDY  0x40
#define ATA_SR_BSY  0x80

#define ATA_TIMEOUT 1000000UL

static unsigned long ata_total_sectors = 0;
static int ata_ready = 0;

static inline void ata_outb(
    unsigned short port,
    unsigned char value
)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline unsigned char ata_inb(
    unsigned short port
)
{
    unsigned char value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static inline void ata_outw(
    unsigned short port,
    unsigned short value
)
{
    __asm__ volatile (
        "outw %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline unsigned short ata_inw(
    unsigned short port
)
{
    unsigned short value;

    __asm__ volatile (
        "inw %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void ata_io_wait(void)
{
    ata_inb(ATA_CONTROL);
    ata_inb(ATA_CONTROL);
    ata_inb(ATA_CONTROL);
    ata_inb(ATA_CONTROL);
}

static int ata_wait_not_busy(void)
{
    for (unsigned long i = 0;
         i < ATA_TIMEOUT;
         i++)
    {
        unsigned char status =
            ata_inb(ATA_STATUS);

        if (status == 0xFF)
            return -1;

        if (status & ATA_SR_ERR)
            return -1;

        if (status & ATA_SR_DF)
            return -1;

        if (!(status & ATA_SR_BSY))
            return 0;
    }

    return -1;
}

static int ata_wait_drq(void)
{
    for (unsigned long i = 0;
         i < ATA_TIMEOUT;
         i++)
    {
        unsigned char status =
            ata_inb(ATA_STATUS);

        if (status == 0xFF)
            return -1;

        if (status & ATA_SR_ERR)
            return -1;

        if (status & ATA_SR_DF)
            return -1;

        if (status & ATA_SR_DRQ)
            return 0;
    }

    return -1;
}

static void ata_select(
    unsigned long lba
)
{
    ata_outb(
        ATA_DRIVE,
        (unsigned char)(
            0xE0 |
            ((lba >> 24) & 0x0F)
        )
    );

    ata_io_wait();
}

static int ata_identify(void)
{
    unsigned short identify[256];

    console_write(
        "ATA: IDENTIFY...\n"
    );

    ata_outb(
        ATA_DRIVE,
        0xE0
    );

    ata_io_wait();

    ata_outb(
        ATA_SECTOR_COUNT,
        0
    );

    ata_outb(
        ATA_LBA_LOW,
        0
    );

    ata_outb(
        ATA_LBA_MID,
        0
    );

    ata_outb(
        ATA_LBA_HIGH,
        0
    );

    ata_outb(
        ATA_COMMAND,
        ATA_CMD_IDENTIFY
    );

    unsigned char status =
        ata_inb(ATA_STATUS);

    console_write(
        "ATA: IDENTIFY status="
    );
    console_write_uint(status);
    console_putc('\n');

    if (status == 0x00 ||
        status == 0xFF)
    {
        console_write(
            "ATA: no device\n"
        );
        return -1;
    }

    if (ata_wait_not_busy() != 0) {
        console_write(
            "ATA: IDENTIFY busy timeout\n"
        );
        return -1;
    }

    unsigned char mid =
        ata_inb(ATA_LBA_MID);

    unsigned char high =
        ata_inb(ATA_LBA_HIGH);

    if (mid != 0 ||
        high != 0)
    {
        console_write(
            "ATA: not an ATA disk\n"
        );
        return -1;
    }

    if (ata_wait_drq() != 0) {
        console_write(
            "ATA: IDENTIFY DRQ timeout\n"
        );
        return -1;
    }

    for (unsigned int i = 0;
         i < 256;
         i++)
    {
        identify[i] =
            ata_inw(ATA_DATA);
    }

    ata_total_sectors =
        ((unsigned long)identify[61] << 16) |
        (unsigned long)identify[60];

    console_write(
        "ATA: sectors="
    );
    console_write_uint(
        ata_total_sectors
    );
    console_putc('\n');

    if (ata_total_sectors == 0)
        return -1;

    console_write(
        "ATA: IDENTIFY OK\n"
    );

    return 0;
}

int ata_init(void)
{
    if (ata_ready)
        return 0;

    console_write(
        "ATA: init\n"
    );

    if (ata_identify() != 0) {
        console_write(
            "ATA: init FAILED\n"
        );
        return -1;
    }

    ata_ready = 1;

    console_write(
        "ATA: init OK\n"
    );

    return 0;
}

unsigned long ata_sector_count(void)
{
    if (!ata_ready) {
        if (ata_init() != 0)
            return 0;
    }

    return ata_total_sectors;
}

int ata_read_sector(
    unsigned long lba,
    void *buffer
)
{
    if (buffer == 0)
        return -1;

    if (!ata_ready) {
        if (ata_init() != 0)
            return -1;
    }

    if (lba >= ata_total_sectors)
        return -1;

    if (lba > 0x0FFFFFFFUL)
        return -1;

    if (ata_wait_not_busy() != 0)
        return -1;

    ata_select(lba);

    ata_outb(
        ATA_SECTOR_COUNT,
        1
    );

    ata_outb(
        ATA_LBA_LOW,
        (unsigned char)(lba & 0xFF)
    );

    ata_outb(
        ATA_LBA_MID,
        (unsigned char)((lba >> 8) & 0xFF)
    );

    ata_outb(
        ATA_LBA_HIGH,
        (unsigned char)((lba >> 16) & 0xFF)
    );

    ata_outb(
        ATA_COMMAND,
        ATA_CMD_READ_PIO
    );

    if (ata_wait_drq() != 0)
        return -1;

    unsigned short *words =
        (unsigned short *)buffer;

    for (unsigned int i = 0;
         i < 256;
         i++)
    {
        words[i] =
            ata_inw(ATA_DATA);
    }

    /*
     * Make sure the device has finished
     * the command before returning.
     */
    if (ata_wait_not_busy() != 0)
        return -1;

    return 0;
}

int ata_write_sector(
    unsigned long lba,
    const void *buffer
)
{
    if (buffer == 0)
        return -1;

    if (!ata_ready) {
        if (ata_init() != 0)
            return -1;
    }

    if (lba >= ata_total_sectors)
        return -1;

    if (lba > 0x0FFFFFFFUL)
        return -1;

    console_write(
        "ATA: write LBA "
    );
    console_write_uint(lba);
    console_putc('\n');

    if (ata_wait_not_busy() != 0) {
        console_write(
            "ATA: busy before write\n"
        );
        return -1;
    }

    ata_select(lba);

    ata_outb(
        ATA_SECTOR_COUNT,
        1
    );

    ata_outb(
        ATA_LBA_LOW,
        (unsigned char)(lba & 0xFF)
    );

    ata_outb(
        ATA_LBA_MID,
        (unsigned char)((lba >> 8) & 0xFF)
    );

    ata_outb(
        ATA_LBA_HIGH,
        (unsigned char)((lba >> 16) & 0xFF)
    );

    ata_outb(
        ATA_COMMAND,
        ATA_CMD_WRITE_PIO
    );

    if (ata_wait_drq() != 0) {
        console_write(
            "ATA: DRQ timeout\n"
        );
        return -1;
    }

    console_write(
        "ATA: DRQ ready\n"
    );

    const unsigned short *words =
        (const unsigned short *)buffer;

    for (unsigned int i = 0;
         i < 256;
         i++)
    {
        ata_outw(
            ATA_DATA,
            words[i]
        );
    }

    console_write(
        "ATA: sector data sent\n"
    );

    /*
     * THIS IS THE IMPORTANT PART.
     *
     * The previous version returned while
     * status was 0xD0 = BSY + DRQ.
     *
     * Wait until the device finishes.
     */
    if (ata_wait_not_busy() != 0) {
        console_write(
            "ATA: write completion timeout\n"
        );
        return -1;
    }

    unsigned char status =
        ata_inb(ATA_STATUS);

    console_write(
        "ATA: write complete status="
    );
    console_write_uint(status);
    console_putc('\n');

    if (status & ATA_SR_ERR) {
        console_write(
            "ATA: write error\n"
        );
        return -1;
    }

    if (status & ATA_SR_DF) {
        console_write(
            "ATA: write device fault\n"
        );
        return -1;
    }

    return 0;
}
