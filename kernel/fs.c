#include "fs.h"
#include "heap.h"
#include "console.h"

#define FS_MAX_FILES 16
#define FS_NAME_SIZE 32
#define FS_FILE_SIZE 4096

struct fs_file
{
    int used;

    char name[FS_NAME_SIZE];

    char *data;

    unsigned long long size;
};

static struct fs_file files[FS_MAX_FILES];


static int string_equal(
    const char *a,
    const char *b
)
{
    while (*a && *b)
    {
        if (*a != *b)
            return 0;

        a++;
        b++;
    }

    return *a == '\0' &&
           *b == '\0';
}


static unsigned long long string_length(
    const char *text
)
{
    unsigned long long length = 0;

    while (text[length])
        length++;

    return length;
}


static void copy_string(
    char *destination,
    const char *source
)
{
    unsigned int i = 0;

    while (source[i] &&
           i < FS_NAME_SIZE - 1)
    {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}


static int find_file(
    const char *name
)
{
    for (int i = 0;
         i < FS_MAX_FILES;
         i++)
    {
        if (files[i].used &&
            string_equal(files[i].name, name))
        {
            return i;
        }
    }

    return -1;
}


static int create_file(
    const char *name
)
{
    if (find_file(name) >= 0)
        return -1;

    for (int i = 0;
         i < FS_MAX_FILES;
         i++)
    {
        if (!files[i].used)
        {
            char *data =
                (char *)kmalloc(
                    FS_FILE_SIZE
                );

            if (data == 0)
                return -1;

            files[i].used = 1;

            copy_string(
                files[i].name,
                name
            );

            files[i].data = data;
            files[i].size = 0;

            data[0] = '\0';

            return i;
        }
    }

    return -1;
}


void fs_init(void)
{
    for (int i = 0;
         i < FS_MAX_FILES;
         i++)
    {
        files[i].used = 0;
        files[i].data = 0;
        files[i].size = 0;
    }


    int file =
        create_file("README");

    if (file >= 0)
    {
        const char *text =
            "TriangleOS RAM filesystem\n";

        unsigned long long length =
            string_length(text);

        for (unsigned long long i = 0;
             i < length;
             i++)
        {
            files[file].data[i] =
                text[i];
        }

        files[file].data[length] = '\0';
        files[file].size = length;
    }
}


void fs_list(void)
{
    console_write(
        "NAME                 SIZE\n"
    );

    for (int i = 0;
         i < FS_MAX_FILES;
         i++)
    {
        if (!files[i].used)
            continue;

        console_write(files[i].name);

        console_write("                 ");

        console_write_uint(
            files[i].size
        );

        console_putc('\n');
    }
}


int fs_write(
    const char *name,
    const char *data
)
{
    unsigned long long length =
        string_length(data);

    if (length >= FS_FILE_SIZE)
        return -1;

    int file =
        find_file(name);

    if (file < 0)
    {
        file = create_file(name);

        if (file < 0)
            return -1;
    }

    for (unsigned long long i = 0;
         i < length;
         i++)
    {
        files[file].data[i] =
            data[i];
    }

    files[file].data[length] = '\0';
    files[file].size = length;

    return 0;
}


void fs_cat(
    const char *name
)
{
    int file =
        find_file(name);

    if (file < 0)
    {
        console_write(
            "File not found.\n"
        );

        return;
    }

    console_write(
        files[file].data
    );

    if (files[file].size == 0 ||
        files[file].data[
            files[file].size - 1
        ] != '\n')
    {
        console_putc('\n');
    }
}


int fs_remove(
    const char *name
)
{
    int file =
        find_file(name);

    if (file < 0)
        return -1;

    kfree(files[file].data);

    files[file].data = 0;
    files[file].size = 0;
    files[file].used = 0;
    files[file].name[0] = '\0';

    return 0;
}
