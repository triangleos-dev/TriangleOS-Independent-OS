bits 64

global default_isr
global timer_isr
global keyboard_isr

extern timer_ticks
extern keyboard_handler

section .text


default_isr:
    cli

.hang:
    hlt
    jmp .hang


timer_isr:
    inc qword [rel timer_ticks]

    mov al, 0x20
    out 0x20, al

    iretq


keyboard_isr:
    cli

    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    cld

    call keyboard_handler

    mov al, 0x20
    out 0x20, al

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    iretq
