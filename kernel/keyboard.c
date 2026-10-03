#include "keyboard.h"
#include "io.h"
#include "shell.h"

static int shift_pressed = 0;

static const char keymap[128] =
{
    0, 0,
    '1','2','3','4','5','6','7','8','9','0','-','=',
    0, 0,
    'q','w','e','r','t','y','u','i','o','p','[',']',
    0, 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\',
    'z','x','c','v','b','n','m',',','.','/',
    0, 0, 0, ' '
};

static const char shiftmap[128] =
{
    0, 0,
    '!','@','#','$','%','^','&','*','(',')','_','+',
    0, 0,
    'Q','W','E','R','T','Y','U','I','O','P','{','}',
    0, 0,
    'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|',
    'Z','X','C','V','B','N','M','<','>','?',
    0, 0, 0, ' '
};

void keyboard_handler(void)
{
    unsigned char scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36)
    {
        shift_pressed = 1;
        return;
    }

    if (scancode == 0xAA || scancode == 0xB6)
    {
        shift_pressed = 0;
        return;
    }

    /* Key release */
    if (scancode & 0x80)
        return;

    /* Enter */
    if (scancode == 0x1C)
    {
        shell_enter();
        return;
    }

    /* Backspace */
    if (scancode == 0x0E)
    {
        shell_backspace();
        return;
    }

    if (scancode >= 128)
        return;

    char c = shift_pressed
        ? shiftmap[scancode]
        : keymap[scancode];

    if (c)
        shell_input(c);
}
