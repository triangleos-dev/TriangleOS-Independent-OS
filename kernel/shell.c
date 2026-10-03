#include "shell.h"
#include "console.h"
#include "memory.h"
#include "timer.h"
#include "rtc.h"
#include "cpu.h"
#include "task.h"
#include "fs.h"
#include "syscall.h"

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
        console_write("  cpu\n");

        console_write("  ls\n");
        console_write("  cat <file>\n");
        console_write("  write <file> <text>\n");
        console_write("  rm <file>\n");

        console_write("  ps\n");
        console_write("  spawn\n");
        console_write("  yield\n");

        console_write("  syscall\n");
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


    else if (string_equals(command, "cpu"))
    {
        const struct cpu_info *info =
            cpu_get_info();

        console_write("Vendor: ");
        console_write(info->vendor);
        console_putc('\n');

        console_write("Family: ");
        console_write_uint(info->family);
        console_putc('\n');

        console_write("Model: ");
        console_write_uint(info->model);
        console_putc('\n');

        console_write("Logical processors: ");
        console_write_uint(
            info->logical_processors
        );
        console_putc('\n');

        console_write("Long mode: ");
        console_write(
            info->long_mode ? "yes" : "no"
        );
        console_putc('\n');
    }


    else if (string_equals(command, "ls"))
    {
        fs_list();
    }


    else if (string_starts_with(command, "cat "))
    {
        fs_cat(command + 4);
    }


    else if (string_starts_with(command, "rm "))
    {
        if (fs_remove(command + 3) == 0)
        {
            console_write("Removed.\n");
        }
        else
        {
            console_write("File not found.\n");
        }
    }


    else if (string_starts_with(command, "write "))
    {
        char *arguments =
            command + 6;

        char *separator = arguments;

        while (*separator &&
               *separator != ' ')
        {
            separator++;
        }

        if (*separator == '\0')
        {
            console_write(
                "Usage: write <file> <text>\n"
            );
        }
        else
        {
            *separator = '\0';
            separator++;

            if (fs_write(
                    arguments,
                    separator
                ) == 0)
            {
                console_write("Written.\n");
            }
            else
            {
                console_write(
                    "Write failed.\n"
                );
            }
        }
    }


    else if (string_equals(command, "ps"))
    {
        task_list();
    }


    else if (string_equals(command, "spawn"))
    {
        task_spawn_demo();
    }


    else if (string_equals(command, "yield"))
    {
        task_yield();
    }


    else if (string_equals(command, "syscall"))
    {
        long result =
            syscall1(
                SYS_WRITE,
                (unsigned long)
                "Hello from int 0x80!\n"
            );

        console_write("Return value: ");
        console_write_uint(
            (unsigned long long)result
        );
        console_write("\n");
    }


    else if (string_equals(command, "echo"))
    {
        console_putc('\n');
    }


    else if (string_starts_with(
        command,
        "echo "))
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
