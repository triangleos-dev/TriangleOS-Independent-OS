#include "console.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

static volatile unsigned short *vga =
    (volatile unsigned short *)VGA_MEMORY;

static unsigned int cursor_x = 0;
static unsigned int cursor_y = 0;


static unsigned short vga_entry(
    char c,
    unsigned char color
)
{
    return ((unsigned short)color << 8) |
           (unsigned char)c;
}


void console_clear(void)
{
    for (unsigned int y = 0;
         y < VGA_HEIGHT;
         y++)
    {
        for (unsigned int x = 0;
             x < VGA_WIDTH;
             x++)
        {
            vga[y * VGA_WIDTH + x] =
                vga_entry(' ', 0x07);
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}


void console_init(void)
{
    console_clear();
}


void console_putc(char c)
{
    if (c == '\n')
    {
        cursor_x = 0;

        if (cursor_y < VGA_HEIGHT - 1)
            cursor_y++;

        return;
    }

    vga[cursor_y * VGA_WIDTH + cursor_x] =
        vga_entry(c, 0x07);

    cursor_x++;

    if (cursor_x >= VGA_WIDTH)
    {
        cursor_x = 0;

        if (cursor_y < VGA_HEIGHT - 1)
            cursor_y++;
    }
}


void console_write(const char *text)
{
    while (*text != '\0')
    {
        console_putc(*text);
        text++;
    }
}


void console_write_uint(
    unsigned long long value
)
{
    char buffer[21];
    unsigned int position = 0;

    if (value == 0)
    {
        console_putc('0');
        return;
    }

    while (value > 0)
    {
        buffer[position++] =
            '0' + (value % 10);

        value /= 10;
    }

    while (position > 0)
    {
        position--;

        console_putc(buffer[position]);
    }
}


void console_backspace(void)
{
    if (cursor_x == 0)
        return;

    cursor_x--;

    vga[cursor_y * VGA_WIDTH + cursor_x] =
        vga_entry(' ', 0x07);
}
