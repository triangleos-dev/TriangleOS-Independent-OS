bits 64

global task_switch

section .text

task_switch:
    ; rdi = old context
    ; rsi = new context

    mov [rdi + 0],  rsp
    mov [rdi + 8],  rbx
    mov [rdi + 16], rbp
    mov [rdi + 24], r12
    mov [rdi + 32], r13
    mov [rdi + 40], r14
    mov [rdi + 48], r15

    mov rsp, [rsi + 0]
    mov rbx, [rsi + 8]
    mov rbp, [rsi + 16]
    mov r12, [rsi + 24]
    mov r13, [rsi + 32]
    mov r14, [rsi + 40]
    mov r15, [rsi + 48]

    ret
