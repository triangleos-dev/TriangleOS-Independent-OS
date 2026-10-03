#ifndef TRIANGLEOS_FS_H
#define TRIANGLEOS_FS_H

void fs_init(void);

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

#endif
