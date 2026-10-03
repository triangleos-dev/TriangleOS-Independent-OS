#include "syscall.h"
#include "console.h"
#include "timer.h"
#include "memory.h"

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
    switch (frame->rax)
    {
        case SYS_WRITE:
        {
            /*
             * Kernel-only for now.
             * User pointer validation comes
             * when user mode is implemented.
             */
            console_write(
                (const char *)frame->rdi
            );

            frame->rax = 0;
            break;
        }


        case SYS_TICKS:
        {
            frame->rax =
                timer_get_ticks();

            break;
        }


        case SYS_FREE_PAGES:
        {
            frame->rax =
                memory_free_pages();

            break;
        }


        default:
        {
            frame->rax =
                (unsigned long long)-1;

            break;
        }
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
