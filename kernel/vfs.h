#ifndef TRIANGLEOS_VFS_H
#define TRIANGLEOS_VFS_H

#define VFS_O_READ    0x01
#define VFS_O_WRITE   0x02
#define VFS_O_CREATE  0x04
#define VFS_O_TRUNC   0x08

#define VFS_SEEK_SET  0
#define VFS_SEEK_CUR  1
#define VFS_SEEK_END  2

int vfs_init(void);

int vfs_open(
    const char *path,
    unsigned int flags
);

int vfs_read(
    int fd,
    void *buffer,
    unsigned long length
);

int vfs_write(
    int fd,
    const void *buffer,
    unsigned long length
);

long vfs_seek(
    int fd,
    long offset,
    int whence
);

int vfs_close(
    int fd
);

int vfs_load(
    const char *path,
    void *buffer,
    unsigned long capacity,
    unsigned long *loaded
);

#endif
