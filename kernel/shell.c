#include "shell.h"
#include "console.h"
#include "memory.h"
#include "timer.h"
#include "rtc.h"

#define COMMAND_SIZE 128

static char command[COMMAND_SIZE];
static unsigned int command_length = 0;


static int string_equals(
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


static int string_starts_with(
    const char *text,
    const char *prefix
)
{
    while (*prefix)
    {
        if (*text != *prefix)
            return 0;

        text++;
        prefix++;
    }

    return 1;
}


static void command_clear(void)
{
    command_length = 0;
    command[0] = '\0';
}


static void print_two_digits(
    unsigned int value
)
{
    console_putc(
        '0' + ((value / 10) % 10)
    );

    console_putc(
        '0' + (value % 10)
    );
}


static void execute_command(void)
{
    if (command_length == 0)
        return;


    if (string_equals(command, "help"))
    {
        console_write("Commands:\n");
        console_write("  help\n");
        console_write("  clear\n");
        console_write("  echo <text>\n");
        console_write("  about\n");
        console_write("  mem\n");
        console_write("  ticks\n");
        console_write("  time\n");
    }


    else if (string_equals(command, "clear"))
    {
        console_clear();
        command_clear();
        return;
    }


    else if (string_equals(command, "about"))
    {
        console_write("TriangleOS\n");
        console_write(
            "Independent x86-64 operating system.\n"
        );

        console_write(
            "Low-level system with direct hardware control.\n"
        );
    }


    else if (string_equals(command, "mem"))
    {
        console_write(
            "E820 usable memory information:\n"
        );

        memory_print_info();
    }


    else if (string_equals(command, "ticks"))
    {
        console_write("Timer ticks: ");

        console_write_uint(
            timer_get_ticks()
        );

        console_write("\n");
    }


    else if (string_equals(command, "time"))
    {
        struct rtc_time time;

        rtc_read(&time);

        console_write_uint(time.year);
        console_putc('-');

        print_two_digits(time.month);
        console_putc('-');

        print_two_digits(time.day);
        console_putc(' ');

        print_two_digits(time.hour);
        console_putc(':');

        print_two_digits(time.minute);
        console_putc(':');

        print_two_digits(time.second);

        console_putc('\n');
    }


    else if (string_equals(command, "echo"))
    {
        console_putc('\n');
    }


    else if (string_starts_with(command, "echo "))
    {
        console_write(command + 5);
        console_putc('\n');
    }


    else
    {
        console_write("Unknown command: ");
        console_write(command);
        console_putc('\n');
    }

    command_clear();
}


void shell_init(void)
{
    command_clear();

    console_write("> ");
}


void shell_input(char c)
{
    if (command_length >= COMMAND_SIZE - 1)
        return;

    command[command_length++] = c;

    command[command_length] = '\0';

    console_putc(c);
}


void shell_backspace(void)
{
    if (command_length == 0)
        return;

    command_length--;

    command[command_length] = '\0';

    console_backspace();
}


void shell_enter(void)
{
    console_putc('\n');

    execute_command();

    console_write("> ");
}
