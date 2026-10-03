#include "console.h"
#include "idt.h"
#include "pic.h"
#include "shell.h"
#include "memory.h"
#include "heap.h"
#include "timer.h"
#include "rtc.h"


static void print_hex(
    unsigned long long value
)
{
    const char *hex =
        "0123456789ABCDEF";

    console_write("0x");

    for (int shift = 60;
         shift >= 0;
         shift -= 4)
    {
        console_putc(
            hex[(value >> shift) & 0xF]
        );
    }
}


void kernel_main(void)
{
    console_init();

    console_write(
        "TriangleOS kernel started!\n"
    );

    console_write(
        "Initializing interrupts...\n"
    );

    idt_init();
    pic_init();

    console_write(
        "Initializing timer...\n"
    );

    timer_init();

    console_write(
        "Timer ready: 100 Hz\n"
    );

    console_write(
        "Initializing RTC...\n"
    );

    rtc_init();

    console_write(
        "RTC ready.\n"
    );

    console_write(
        "Keyboard ready.\n"
    );

    console_write(
        "Initializing physical memory...\n"
    );

    memory_init();

    memory_print_info();


    void *page = page_alloc();

    if (page != 0)
    {
        console_write("Allocated page: ");

        print_hex(
            (unsigned long long)page
        );

        console_putc('\n');

        page_free(page);

        console_write(
            "Page successfully freed.\n"
        );
    }
    else
    {
        console_write(
            "ERROR: page allocation failed.\n"
        );
    }


    heap_init();


    char *buffer =
        (char *)kmalloc(128);

    if (buffer != 0)
    {
        buffer[0] = 'H';
        buffer[1] = 'e';
        buffer[2] = 'a';
        buffer[3] = 'p';
        buffer[4] = ' ';
        buffer[5] = 'O';
        buffer[6] = 'K';
        buffer[7] = '\0';

        console_write("Heap test: ");
        console_write(buffer);
        console_putc('\n');

        kfree(buffer);

        console_write(
            "Heap block freed.\n"
        );
    }
    else
    {
        console_write(
            "ERROR: heap allocation failed.\n"
        );
    }


    shell_init();

    __asm__ volatile ("sti");

    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}
