bits 64

global task_switch

section .text

; void task_switch(struct task_context *old,
;                  struct task_context *new)
;
; struct task_context contains:
;   +0   rsp
;   +8   rbx
;   +16  rbp
;   +24  r12
;   +32  r13
;   +40  r14
;   +48  r15
;
; The saved stack itself contains:
;   r15
;   r14
;   r13
;   r12
;   rbp
;   rbx
;   return RIP
;
; RDI = old context
; RSI = new context

task_switch:
    ; Save callee-saved registers of current task.
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; Save current task's stack pointer.
    mov [rdi], rsp

    ; Load next task's stack pointer.
    mov rsp, [rsi]

    ; Restore next task's callee-saved registers.
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ; Return to the next task.
    ret
