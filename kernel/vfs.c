#include "vfs.h"
#include "fs.h"

#define VFS_MAX_FDS 32

struct vfs_fd
{
    int used;
    unsigned int flags;
    unsigned long offset;
    char name[64];
};

static struct vfs_fd descriptors[
    VFS_MAX_FDS
];

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

static void copy_name(
    char *destination,
    const char *source
)
{
    unsigned long length =
        string_length(source);

    if (length >= 64)
        length = 63;

    for (
        unsigned long i = 0;
        i < length;
        i++
    )
    {
        destination[i] = source[i];
    }

    destination[length] = '\0';
}

static const char *normalize_path(
    const char *path
)
{
    if (path == 0)
        return 0;

    while (*path == '/')
        path++;

    return path;
}

int vfs_init(void)
{
    fs_init();

    return fs_exists(
        "README.TXT"
    ) ? 0 : -1;
}

int vfs_open(
    const char *path,
    unsigned int flags
)
{
    const char *name =
        normalize_path(path);

    if (name == 0 || *name == '\0')
        return -1;

    if (!fs_exists(name)) {
        if (!(flags & VFS_O_CREATE))
            return -1;

        if (
            fs_create(name) != 0
        )
            return -1;
    }

    if (flags & VFS_O_TRUNC) {
        if (
            !(flags & VFS_O_WRITE) ||
            fs_truncate(name) != 0
        )
            return -1;
    }

    for (
        int i = 0;
        i < VFS_MAX_FDS;
        i++
    )
    {
        if (descriptors[i].used)
            continue;

        descriptors[i].used = 1;
        descriptors[i].flags = flags;
        descriptors[i].offset = 0;

        copy_name(
            descriptors[i].name,
            name
        );

        return i;
    }

    return -1;
}

int vfs_read(
    int fd,
    void *buffer,
    unsigned long length
)
{
    if (
        fd < 0 ||
        fd >= VFS_MAX_FDS ||
        buffer == 0 ||
        !descriptors[fd].used ||
        !(descriptors[fd].flags &
          VFS_O_READ)
    )
        return -1;

    int result =
        fs_read_at(
            descriptors[fd].name,
            descriptors[fd].offset,
            buffer,
            length
        );

    if (result < 0)
        return -1;

    unsigned long size;

    if (
        fs_get_size(
            descriptors[fd].name,
            &size
        ) != 0
    )
        return -1;

    unsigned long remaining =
        size > descriptors[fd].offset
            ? size - descriptors[fd].offset
            : 0;

    unsigned long actual =
        remaining < length
            ? remaining
            : length;

    descriptors[fd].offset += actual;

    return (int)actual;
}

int vfs_write(
    int fd,
    const void *buffer,
    unsigned long length
)
{
    if (
        fd < 0 ||
        fd >= VFS_MAX_FDS ||
        buffer == 0 ||
        !descriptors[fd].used ||
        !(descriptors[fd].flags &
          VFS_O_WRITE)
    )
        return -1;

    if (
        fs_write_at(
            descriptors[fd].name,
            descriptors[fd].offset,
            buffer,
            length
        ) != 0
    )
        return -1;

    descriptors[fd].offset += length;

    return (int)length;
}

long vfs_seek(
    int fd,
    long offset,
    int whence
)
{
    if (
        fd < 0 ||
        fd >= VFS_MAX_FDS ||
        !descriptors[fd].used
    )
        return -1;

    unsigned long base;

    if (whence == VFS_SEEK_SET) {
        base = 0;
    }
    else if (whence == VFS_SEEK_CUR) {
        base = descriptors[fd].offset;
    }
    else if (whence == VFS_SEEK_END) {
        if (
            fs_get_size(
                descriptors[fd].name,
                &base
            ) != 0
        )
            return -1;
    }
    else {
        return -1;
    }

    if (offset < 0) {
        unsigned long magnitude =
            (unsigned long)(-offset);

        if (magnitude > base)
            return -1;

        descriptors[fd].offset =
            base - magnitude;
    }
    else {
        descriptors[fd].offset =
            base + (unsigned long)offset;
    }

    return (long)descriptors[fd].offset;
}

int vfs_close(
    int fd
)
{
    if (
        fd < 0 ||
        fd >= VFS_MAX_FDS ||
        !descriptors[fd].used
    )
        return -1;

    descriptors[fd].used = 0;
    descriptors[fd].flags = 0;
    descriptors[fd].offset = 0;
    descriptors[fd].name[0] = '\0';

    return 0;
}

int vfs_load(
    const char *path,
    void *buffer,
    unsigned long capacity,
    unsigned long *loaded
)
{
    return fs_load(
        normalize_path(path),
        buffer,
        capacity,
        loaded
    );
}
