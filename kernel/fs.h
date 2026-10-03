#ifndef TRIANGLEOS_FS_H
#define TRIANGLEOS_FS_H

void fs_init(void);

int fs_mount(void);
int fs_format(void);

void fs_list(void);

int fs_write(
    const char *name,
    const char *data
);

void fs_cat(
    const char *name
);

int fs_remove(
    const char *name
);

int fs_exists(
    const char *name
);

int fs_create(
    const char *name
);

int fs_truncate(
    const char *name
);

int fs_read_at(
    const char *name,
    unsigned long offset,
    void *buffer,
    unsigned long length
);

int fs_get_size(
    const char *name,
    unsigned long *size
);

int fs_write_at(
    const char *name,
    unsigned long offset,
    const void *buffer,
    unsigned long length
);

int fs_load(
    const char *name,
    void *buffer,
    unsigned long capacity,
    unsigned long *loaded
);

#endif
