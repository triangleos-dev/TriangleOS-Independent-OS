bits 64

global default_isr
global timer_isr
global keyboard_isr
global syscall_isr

extern timer_ticks
extern keyboard_handler
extern syscall_dispatch

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

    cld

    call keyboard_handler

    mov al, 0x20
    out 0x20, al

    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    pop rax

    iretq


syscall_isr:

    push rax
    push rbx
    push rcx
    push rdx
    push rbp
    push rsi
    push rdi

    mov rdi, rsp

    call syscall_dispatch

    ; syscall_dispatch put the result in frame->rax
    mov rax, [rsp + 0]

    pop rdi
    pop rsi
    pop rbp
    pop rdx
    pop rcx
    pop rbx
    ; Do not restore the old RAX.
    ; We want the syscall return value.
    add rsp, 8

    iretq
