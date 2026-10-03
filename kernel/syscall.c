#include "syscall.h"
#include "console.h"
#include "timer.h"
#include "memory.h"
#include "user.h"

struct syscall_frame
{
    unsigned long long rax;
    unsigned long long rbx;
    unsigned long long rcx;
    unsigned long long rdx;
    unsigned long long rbp;
    unsigned long long rsi;
    unsigned long long rdi;
    unsigned long long r8;
    unsigned long long r9;
    unsigned long long r10;
    unsigned long long r11;
    unsigned long long r12;
    unsigned long long r13;
    unsigned long long r14;
    unsigned long long r15;
};

void syscall_dispatch(
    struct syscall_frame *frame
)
{
    if (frame == 0)
        return;

    switch (frame->rax)
    {
        case SYS_WRITE:
        {
            char text[256];

            int length =
                user_copy_string(
                    (const char *)frame->rdi,
                    text,
                    sizeof(text)
                );

            if (length < 0)
            {
                frame->rax =
                    (unsigned long long)-1;

                break;
            }

            console_write(text);

            frame->rax =
                (unsigned long long)length;

            break;
        }

        case SYS_TICKS:

            frame->rax =
                timer_get_ticks();

            break;

        case SYS_FREE_PAGES:

            frame->rax =
                memory_free_pages();

            break;

        case SYS_EXIT:

            frame->rax = 0;

            user_exit_to_kernel();

            for (;;)
                __asm__ volatile ("hlt");

        default:

            frame->rax =
                (unsigned long long)-1;

            break;
    }
}

long syscall0(
    unsigned long number
)
{
    long result;

    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(number)
        : "memory", "rcx", "r11"
    );

    return result;
}

long syscall1(
    unsigned long number,
    unsigned long argument
)
{
    long result;

    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "a"(number),
          "D"(argument)
        : "memory", "rcx", "r11"
    );

    return result;
}
