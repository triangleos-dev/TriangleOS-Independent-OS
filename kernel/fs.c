#include "fs.h"
#include "block.h"
#include "console.h"

#define TRIANGLEFS_MAGIC              0x54524653UL
#define TRIANGLEFS_VERSION            1UL

#define TRIANGLEFS_START_LBA          128UL
#define TRIANGLEFS_SUPERBLOCK_LBA     TRIANGLEFS_START_LBA

#define TRIANGLEFS_DIR_LBA            (TRIANGLEFS_START_LBA + 1UL)
#define TRIANGLEFS_DIR_SECTORS        8UL

#define TRIANGLEFS_DATA_LBA           \
    (TRIANGLEFS_DIR_LBA + TRIANGLEFS_DIR_SECTORS)

#define TRIANGLEFS_MAX_FILES          64UL
#define TRIANGLEFS_MAX_FILE_SIZE      65536UL
#define TRIANGLEFS_FILE_SECTORS       128UL

#define TRIANGLEFS_SECTOR_SIZE        512UL
#define TRIANGLEFS_NAME_SIZE          31UL
#define TRIANGLEFS_DIR_ENTRY_SIZE     64UL

#define TRIANGLEFS_FILE_USED_OFFSET   0UL
#define TRIANGLEFS_FILE_NAME_OFFSET   1UL
#define TRIANGLEFS_FILE_SIZE_OFFSET   32UL
#define TRIANGLEFS_FILE_DATA_OFFSET   36UL

#define TRIANGLEFS_FILE_USED          0x01U

static unsigned char fs_buffer[
    TRIANGLEFS_SECTOR_SIZE
];

static int fs_ready = 0;

static unsigned long string_length(
    const char *text
)
{
    unsigned long length = 0;

    if (text == 0)
        return 0;

    while (text[length] != '\0')
        length++;

    return length;
}

static int string_equal(
    const char *a,
    const char *b
)
{
    if (a == 0 || b == 0)
        return 0;

    while (*a != '\0' &&
           *b != '\0')
    {
        if (*a != *b)
            return 0;

        a++;
        b++;
    }

    return *a == '\0' &&
           *b == '\0';
}

static void memory_zero(
    void *buffer,
    unsigned long length
)
{
    unsigned char *bytes =
        (unsigned char *)buffer;

    for (unsigned long i = 0;
         i < length;
         i++)
    {
        bytes[i] = 0;
    }
}

static void memory_copy(
    void *destination,
    const void *source,
    unsigned long length
)
{
    unsigned char *dst =
        (unsigned char *)destination;

    const unsigned char *src =
        (const unsigned char *)source;

    for (unsigned long i = 0;
         i < length;
         i++)
    {
        dst[i] = src[i];
    }
}

static void put_u32(
    unsigned char *buffer,
    unsigned long offset,
    unsigned long value
)
{
    buffer[offset + 0] =
        (unsigned char)(value & 0xFF);

    buffer[offset + 1] =
        (unsigned char)((value >> 8) & 0xFF);

    buffer[offset + 2] =
        (unsigned char)((value >> 16) & 0xFF);

    buffer[offset + 3] =
        (unsigned char)((value >> 24) & 0xFF);
}

static unsigned long get_u32(
    const unsigned char *buffer,
    unsigned long offset
)
{
    return
        ((unsigned long)buffer[offset + 0]) |
        ((unsigned long)buffer[offset + 1] << 8) |
        ((unsigned long)buffer[offset + 2] << 16) |
        ((unsigned long)buffer[offset + 3] << 24);
}

static unsigned long directory_lba(
    unsigned long index
)
{
    return TRIANGLEFS_DIR_LBA +
           (index / 8UL);
}

static unsigned long directory_offset(
    unsigned long index
)
{
    return
        (index % 8UL) *
        TRIANGLEFS_DIR_ENTRY_SIZE;
}

static unsigned long file_data_lba(
    unsigned long index
)
{
    return TRIANGLEFS_DATA_LBA +
           index * TRIANGLEFS_FILE_SECTORS;
}

static int read_sector(
    unsigned long lba,
    void *buffer
)
{
    return block_read(
        lba,
        buffer
    );
}

static int write_sector(
    unsigned long lba,
    const void *buffer
)
{
    return block_write(
        lba,
        buffer
    );
}

static int filesystem_ready(void)
{
    if (fs_ready)
        return 1;

    if (fs_mount() != 0)
        return 0;

    return 1;
}

static int load_directory_sector(
    unsigned long sector,
    unsigned char *buffer
)
{
    return read_sector(
        TRIANGLEFS_DIR_LBA + sector,
        buffer
    );
}

static int save_directory_sector(
    unsigned long sector,
    const unsigned char *buffer
)
{
    return write_sector(
        TRIANGLEFS_DIR_LBA + sector,
        buffer
    );
}

static int read_entry(
    unsigned long index,
    unsigned char *entry
)
{
    unsigned long sector =
        index / 8UL;

    unsigned long offset =
        directory_offset(index);

    if (load_directory_sector(
            sector,
            fs_buffer) != 0)
        return -1;

    memory_copy(
        entry,
        fs_buffer + offset,
        TRIANGLEFS_DIR_ENTRY_SIZE
    );

    return 0;
}

static int write_entry(
    unsigned long index,
    const unsigned char *entry
)
{
    unsigned long sector =
        index / 8UL;

    unsigned long offset =
        directory_offset(index);

    if (load_directory_sector(
            sector,
            fs_buffer) != 0)
        return -1;

    memory_copy(
        fs_buffer + offset,
        entry,
        TRIANGLEFS_DIR_ENTRY_SIZE
    );

    return save_directory_sector(
        sector,
        fs_buffer
    );
}

static int find_file(
    const char *name,
    unsigned long *index
)
{
    unsigned char entry[
        TRIANGLEFS_DIR_ENTRY_SIZE
    ];

    for (unsigned long i = 0;
         i < TRIANGLEFS_MAX_FILES;
         i++)
    {
        if (read_entry(i, entry) != 0)
            return -1;

        if (entry[
                TRIANGLEFS_FILE_USED_OFFSET
            ] != TRIANGLEFS_FILE_USED)
            continue;

        if (string_equal(
                name,
                (const char *)(
                    entry +
                    TRIANGLEFS_FILE_NAME_OFFSET)))
        {
            if (index != 0)
                *index = i;

            return 0;
        }
    }

    return -1;
}

static int find_free_file(
    unsigned long *index
)
{
    unsigned char entry[
        TRIANGLEFS_DIR_ENTRY_SIZE
    ];

    for (unsigned long i = 0;
         i < TRIANGLEFS_MAX_FILES;
         i++)
    {
        if (read_entry(i, entry) != 0)
            return -1;

        if (entry[
                TRIANGLEFS_FILE_USED_OFFSET
            ] == 0)
        {
            if (index != 0)
                *index = i;

            return 0;
        }
    }

    return -1;
}

static int superblock_valid(void)
{
    if (read_sector(
            TRIANGLEFS_SUPERBLOCK_LBA,
            fs_buffer) != 0)
        return 0;

    if (get_u32(fs_buffer, 0) !=
        TRIANGLEFS_MAGIC)
        return 0;

    if (get_u32(fs_buffer, 4) !=
        TRIANGLEFS_VERSION)
        return 0;

    if (get_u32(fs_buffer, 8) !=
        TRIANGLEFS_SECTOR_SIZE)
        return 0;

    if (get_u32(fs_buffer, 12) !=
        TRIANGLEFS_MAX_FILES)
        return 0;

    if (get_u32(fs_buffer, 16) !=
        TRIANGLEFS_MAX_FILE_SIZE)
        return 0;

    return 1;
}

int fs_mount(void)
{
    if (fs_ready)
        return 0;

    console_write(
        "TriangleFS: mounting...\n"
    );

    if (block_init() != 0)
    {
        console_write(
            "TriangleFS: block device unavailable\n"
        );

        return -1;
    }

    if (!superblock_valid())
    {
        console_write(
            "TriangleFS: no valid filesystem\n"
        );

        return -1;
    }

    fs_ready = 1;

    console_write(
        "TriangleFS: mounted\n"
    );

    return 0;
}

static int create_readme(void)
{
    if (fs_create("README.TXT") != 0)
        return -1;

    if (fs_write(
            "README.TXT",
            "TriangleOS filesystem\n"
            "Persistent disk storage is working.\n"
        ) != 0)
        return -1;

    return 0;
}

int fs_format(void)
{
    console_write(
        "TriangleFS: FORMAT START\n"
    );

    fs_ready = 0;

    if (block_init() != 0)
    {
        console_write(
            "TriangleFS: block device unavailable\n"
        );

        return -1;
    }

    memory_zero(
        fs_buffer,
        TRIANGLEFS_SECTOR_SIZE
    );

    put_u32(
        fs_buffer,
        0,
        TRIANGLEFS_MAGIC
    );

    put_u32(
        fs_buffer,
        4,
        TRIANGLEFS_VERSION
    );

    put_u32(
        fs_buffer,
        8,
        TRIANGLEFS_SECTOR_SIZE
    );

    put_u32(
        fs_buffer,
        12,
        TRIANGLEFS_MAX_FILES
    );

    put_u32(
        fs_buffer,
        16,
        TRIANGLEFS_MAX_FILE_SIZE
    );

    put_u32(
        fs_buffer,
        20,
        TRIANGLEFS_DIR_LBA
    );

    put_u32(
        fs_buffer,
        24,
        TRIANGLEFS_DIR_SECTORS
    );

    put_u32(
        fs_buffer,
        28,
        TRIANGLEFS_DATA_LBA
    );

    console_write(
        "TriangleFS: writing superblock\n"
    );

    if (write_sector(
            TRIANGLEFS_SUPERBLOCK_LBA,
            fs_buffer) != 0)
    {
        console_write(
            "TriangleFS: superblock FAILED\n"
        );

        return -1;
    }

    for (unsigned long sector = 0;
         sector < TRIANGLEFS_DIR_SECTORS;
         sector++)
    {
        memory_zero(
            fs_buffer,
            TRIANGLEFS_SECTOR_SIZE
        );

        console_write(
            "TriangleFS: clearing directory LBA "
        );

        console_write_uint(
            TRIANGLEFS_DIR_LBA + sector
        );

        console_putc('\n');

        if (write_sector(
                TRIANGLEFS_DIR_LBA + sector,
                fs_buffer) != 0)
        {
            console_write(
                "TriangleFS: directory FAILED\n"
            );

            return -1;
        }
    }

    fs_ready = 1;

    if (create_readme() != 0)
    {
        fs_ready = 0;

        console_write(
            "TriangleFS: README creation FAILED\n"
        );

        return -1;
    }

    console_write(
        "TriangleFS: FORMAT COMPLETE\n"
    );

    return 0;
}

void fs_init(void)
{
    if (fs_ready)
        return;

    if (fs_mount() != 0)
    {
        console_write(
            "TriangleFS: not mounted\n"
        );
    }
}

int fs_exists(
    const char *name
)
{
    if (name == 0 || *name == '\0')
        return 0;

    if (!filesystem_ready())
        return 0;

    return find_file(
        name,
        0
    ) == 0;
}

int fs_create(
    const char *name
)
{
    unsigned long index;

    unsigned char entry[
        TRIANGLEFS_DIR_ENTRY_SIZE
    ];

    if (name == 0 || *name == '\0')
        return -1;

    if (string_length(name) >
        TRIANGLEFS_NAME_SIZE)
        return -1;

    if (!filesystem_ready())
        return -1;

    if (find_file(name, 0) == 0)
        return 0;

    if (find_free_file(&index) != 0)
        return -1;

    memory_zero(
        entry,
        TRIANGLEFS_DIR_ENTRY_SIZE
    );

    entry[
        TRIANGLEFS_FILE_USED_OFFSET
    ] = TRIANGLEFS_FILE_USED;

    memory_copy(
        entry +
        TRIANGLEFS_FILE_NAME_OFFSET,
        name,
        string_length(name)
    );

    put_u32(
        entry,
        TRIANGLEFS_FILE_SIZE_OFFSET,
        0
    );

    put_u32(
        entry,
        TRIANGLEFS_FILE_DATA_OFFSET,
        file_data_lba(index)
    );

    return write_entry(
        index,
        entry
    );
}

int fs_truncate(
    const char *name
)
{
    unsigned long index;

    unsigned char entry[
        TRIANGLEFS_DIR_ENTRY_SIZE
    ];

    if (!filesystem_ready())
        return -1;

    if (find_file(
            name,
            &index) != 0)
        return -1;

    if (read_entry(
            index,
            entry) != 0)
        return -1;

    put_u32(
        entry,
        TRIANGLEFS_FILE_SIZE_OFFSET,
        0
    );

    return write_entry(
        index,
        entry
    );
}

int fs_get_size(
    const char *name,
    unsigned long *size
)
{
    unsigned long index;

    unsigned char entry[
        TRIANGLEFS_DIR_ENTRY_SIZE
    ];

    if (size == 0)
        return -1;

    if (!filesystem_ready())
        return -1;

    if (find_file(
            name,
            &index) != 0)
        return -1;

    if (read_entry(
            index,
            entry) != 0)
        return -1;

    *size = get_u32(
        entry,
        TRIANGLEFS_FILE_SIZE_OFFSET
    );

    return 0;
}

static int write_range(
    unsigned long file_index,
    unsigned long offset,
    const unsigned char *source,
    unsigned long length
)
{
    unsigned char sector_buffer[
        TRIANGLEFS_SECTOR_SIZE
    ];

    unsigned long lba =
        file_data_lba(file_index) +
        (offset / TRIANGLEFS_SECTOR_SIZE);

    unsigned long position =
        offset %
        TRIANGLEFS_SECTOR_SIZE;

    unsigned long remaining =
        length;

    while (remaining > 0)
    {
        unsigned long chunk =
            TRIANGLEFS_SECTOR_SIZE -
            position;

        if (chunk > remaining)
            chunk = remaining;

        if (position != 0 ||
            chunk != TRIANGLEFS_SECTOR_SIZE)
        {
            if (read_sector(
                    lba,
                    sector_buffer) != 0)
                return -1;
        }
        else
        {
            memory_zero(
                sector_buffer,
                TRIANGLEFS_SECTOR_SIZE
            );
        }

        memory_copy(
            sector_buffer + position,
            source,
            chunk
        );

        if (write_sector(
                lba,
                sector_buffer) != 0)
            return -1;

        source += chunk;
        remaining -= chunk;
        lba++;
        position = 0;
    }

    return 0;
}

static int zero_range(
    unsigned long file_index,
    unsigned long offset,
    unsigned long length
)
{
    unsigned char zeroes[
        TRIANGLEFS_SECTOR_SIZE
    ];

    memory_zero(
        zeroes,
        TRIANGLEFS_SECTOR_SIZE
    );

    unsigned long remaining =
        length;

    while (remaining > 0)
    {
        unsigned long chunk =
            remaining;

        if (chunk >
            TRIANGLEFS_SECTOR_SIZE)
            chunk =
                TRIANGLEFS_SECTOR_SIZE;

        if (write_range(
                file_index,
                offset,
                zeroes,
                chunk) != 0)
            return -1;

        offset += chunk;
        remaining -= chunk;
    }

    return 0;
}

int fs_read_at(
    const char *name,
    unsigned long offset,
    void *buffer,
    unsigned long length
)
{
    unsigned long index;
    unsigned long size;

    unsigned char sector_buffer[
        TRIANGLEFS_SECTOR_SIZE
    ];

    if (buffer == 0)
        return -1;

    if (!filesystem_ready())
        return -1;

    if (find_file(
            name,
            &index) != 0)
        return -1;

    if (fs_get_size(
            name,
            &size) != 0)
        return -1;

    if (offset >= size)
        return 0;

    if (length > size - offset)
        length = size - offset;

    unsigned long lba =
        file_data_lba(index) +
        offset / TRIANGLEFS_SECTOR_SIZE;

    unsigned long position =
        offset % TRIANGLEFS_SECTOR_SIZE;

    unsigned long remaining =
        length;

    unsigned char *destination =
        (unsigned char *)buffer;

    while (remaining > 0)
    {
        if (read_sector(
                lba,
                sector_buffer) != 0)
            return -1;

        unsigned long chunk =
            TRIANGLEFS_SECTOR_SIZE -
            position;

        if (chunk > remaining)
            chunk = remaining;

        memory_copy(
            destination,
            sector_buffer + position,
            chunk
        );

        destination += chunk;
        remaining -= chunk;
        lba++;
        position = 0;
    }

    return (int)length;
}

int fs_write_at(
    const char *name,
    unsigned long offset,
    const void *buffer,
    unsigned long length
)
{
    unsigned long index;
    unsigned long old_size;
    unsigned long new_size;

    if (buffer == 0)
        return -1;

    if (!filesystem_ready())
        return -1;

    if (find_file(
            name,
            &index) != 0)
        return -1;

    if (fs_get_size(
            name,
            &old_size) != 0)
        return -1;

    if (offset > TRIANGLEFS_MAX_FILE_SIZE)
        return -1;

    if (length >
        TRIANGLEFS_MAX_FILE_SIZE - offset)
        return -1;

    new_size =
        offset + length;

    if (offset > old_size)
    {
        if (zero_range(
                index,
                old_size,
                offset - old_size) != 0)
            return -1;
    }

    if (length > 0)
    {
        if (write_range(
                index,
                offset,
                (const unsigned char *)buffer,
                length) != 0)
            return -1;
    }

    if (new_size != old_size)
    {
        unsigned char entry[
            TRIANGLEFS_DIR_ENTRY_SIZE
        ];

        if (read_entry(
                index,
                entry) != 0)
            return -1;

        put_u32(
            entry,
            TRIANGLEFS_FILE_SIZE_OFFSET,
            new_size
        );

        if (write_entry(
                index,
                entry) != 0)
            return -1;
    }

    return 0;
}

int fs_write(
    const char *name,
    const char *data
)
{
    unsigned long length;

    if (name == 0 ||
        data == 0)
        return -1;

    if (!filesystem_ready())
        return -1;

    if (!fs_exists(name))
    {
        if (fs_create(name) != 0)
            return -1;
    }

    if (fs_truncate(name) != 0)
        return -1;

    length =
        string_length(data);

    if (length == 0)
        return 0;

    return fs_write_at(
        name,
        0,
        data,
        length
    );
}

void fs_cat(
    const char *name
)
{
    unsigned long size;

    unsigned char buffer[
        TRIANGLEFS_SECTOR_SIZE
    ];

    if (!filesystem_ready())
        return;

    if (fs_get_size(
            name,
            &size) != 0)
    {
        console_write(
            "File not found\n"
        );

        return;
    }

    unsigned long offset = 0;

    while (offset < size)
    {
        unsigned long chunk =
            size - offset;

        if (chunk >
            TRIANGLEFS_SECTOR_SIZE)
            chunk =
                TRIANGLEFS_SECTOR_SIZE;

        if (fs_read_at(
                name,
                offset,
                buffer,
                chunk) != (int)chunk)
        {
            console_write(
                "Read error\n"
            );

            return;
        }

        for (unsigned long i = 0;
             i < chunk;
             i++)
        {
            console_putc(
                (char)buffer[i]
            );
        }

        offset += chunk;
    }
}

void fs_list(void)
{
    unsigned char entry[
        TRIANGLEFS_DIR_ENTRY_SIZE
    ];

    if (!filesystem_ready())
    {
        console_write(
            "Filesystem not mounted.\n"
        );

        return;
    }

    console_write(
        "TriangleFS files:\n"
    );

    for (unsigned long i = 0;
         i < TRIANGLEFS_MAX_FILES;
         i++)
    {
        if (read_entry(i, entry) != 0)
            return;

        if (entry[
                TRIANGLEFS_FILE_USED_OFFSET
            ] != TRIANGLEFS_FILE_USED)
            continue;

        console_write("  ");

        console_write(
            (const char *)(
                entry +
                TRIANGLEFS_FILE_NAME_OFFSET
            )
        );

        console_write("  ");

        console_write_uint(
            get_u32(
                entry,
                TRIANGLEFS_FILE_SIZE_OFFSET
            )
        );

        console_write(
            " bytes\n"
        );
    }
}

int fs_remove(
    const char *name
)
{
    unsigned long index;

    unsigned char entry[
        TRIANGLEFS_DIR_ENTRY_SIZE
    ];

    if (!filesystem_ready())
        return -1;

    if (find_file(
            name,
            &index) != 0)
        return -1;

    if (read_entry(
            index,
            entry) != 0)
        return -1;

    memory_zero(
        entry,
        TRIANGLEFS_DIR_ENTRY_SIZE
    );

    return write_entry(
        index,
        entry
    );
}

int fs_load(
    const char *name,
    void *buffer,
    unsigned long capacity,
    unsigned long *loaded
)
{
    unsigned long size;

    if (buffer == 0 ||
        loaded == 0)
        return -1;

    *loaded = 0;

    if (!filesystem_ready())
        return -1;

    if (fs_get_size(
            name,
            &size) != 0)
        return -1;

    if (size > capacity)
        size = capacity;

    if (size == 0)
        return 0;

    int result =
        fs_read_at(
            name,
            0,
            buffer,
            size
        );

    if (result < 0)
        return -1;

    *loaded =
        (unsigned long)result;

    return 0;
}
