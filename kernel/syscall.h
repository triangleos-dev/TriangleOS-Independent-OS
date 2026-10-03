#ifndef TRIANGLEOS_SYSCALL_H
#define TRIANGLEOS_SYSCALL_H

#define SYS_WRITE      1
#define SYS_TICKS      2
#define SYS_FREE_PAGES 3
#define SYS_EXIT       4

long syscall0(unsigned long number);

long syscall1(
    unsigned long number,
    unsigned long argument
);

#endif
