bits 64

global _start
extern kernel_main

section .text

_start:
    cli
    cld

    mov rsp, 0x90000

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang
