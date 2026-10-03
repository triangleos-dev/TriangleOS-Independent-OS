#include "console.h"
#include "idt.h"
#include "pic.h"
#include "shell.h"
#include "memory.h"
#include "heap.h"
#include "timer.h"
#include "rtc.h"
#include "cpu.h"
#include "task.h"
#include "fs.h"


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


    cpu_init();


    idt_init();
    pic_init();


    timer_init();


    rtc_init();


    memory_init();


    void *page =
        page_alloc();

    if (page != 0)
    {
        console_write("Page allocator: OK at ");

        print_hex(
            (unsigned long long)page
        );

        console_putc('\n');

        page_free(page);
    }
    else
    {
        console_write(
            "Page allocator: FAILED\n"
        );
    }


    heap_init();


    char *buffer =
        (char *)kmalloc(64);

    if (buffer != 0)
    {
        buffer[0] = 'H';
        buffer[1] = 'e';
        buffer[2] = 'a';
        buffer[3] = 'p';
        buffer[4] = ' ';
        buffer[5] = 'O';
        buffer[6] = 'O';
        buffer[7] = 'K';
        buffer[8] = '\0';

        console_write(buffer);
        console_putc('\n');

        kfree(buffer);
    }


    task_init();

    console_write(
        "Task manager: ready\n"
    );


    fs_init();

    console_write(
        "RAM filesystem: ready\n"
    );


    console_write(
        "System call interface: ready\n"
    );


    shell_init();


    __asm__ volatile ("sti");


    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}
