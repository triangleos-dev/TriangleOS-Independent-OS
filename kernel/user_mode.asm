bits 64
default rel

global user_enter
global user_exit_to_kernel
global user_syscall_entry

extern syscall_dispatch

extern user_kernel_cr3
extern user_kernel_saved_rsp
extern user_kernel_return
extern user_kernel_stack_top

section .text

user_enter:
    cli

    mov rax, cr3
    mov [user_kernel_cr3], rax

    mov [user_target_cr3], rdi
    mov [user_target_rip], rsi
    mov [user_target_rsp], rdx

    push r15
    push r14
    push r13
    push r12
    push rbp
    push rbx

    mov [user_kernel_saved_rsp], rsp

    lea rax, [.returned]
    mov [user_kernel_return], rax

    call .setup_cpu

    mov rax, [user_kernel_stack_top]
    mov [tss + 4], rax

    mov word [tss + 102], 104

    mov rax, [user_target_rsp]

    push qword 0x2B
    push rax
    push qword 0x202
    push qword 0x23

    mov rax, [user_target_rip]
    push rax

    mov rax, [user_target_cr3]
    mov cr3, rax

    iretq


.returned:
    cli

    mov rax, [user_kernel_cr3]
    mov cr3, rax

    mov rsp, [user_kernel_saved_rsp]

    pop rbx
    pop rbp
    pop r12
    pop r13
    pop r14
    pop r15

    sti

    ret


.setup_cpu:

    cmp byte [gdt_ready], 1
    je .ready

    ;
    ; Build the 64-bit TSS descriptor at runtime.
    ;
    lea rax, [tss]

    mov word [gdt_tss + 0], 0x0067
    mov word [gdt_tss + 2], ax

    shr rax, 16

    mov byte [gdt_tss + 4], al
    mov byte [gdt_tss + 5], 0x89
    mov byte [gdt_tss + 6], 0x00
    mov byte [gdt_tss + 7], ah

    shr rax, 16

    mov dword [gdt_tss + 8], eax
    mov dword [gdt_tss + 12], 0

    ;
    ; Load GDT.
    ;
    lgdt [gdt_descriptor]

    ;
    ; Reload CS.
    ;
    push qword 0x18

    lea rax, [.after_cs_reload]
    push rax

    retfq


.after_cs_reload:

    mov ax, 0x10

    mov ds, ax
    mov es, ax
    mov ss, ax

    ;
    ; Load TSS selector.
    ;
    mov ax, 0x30
    ltr ax

    ;
    ; Enable SYSCALL/SYSRET.
    ;
    mov ecx, 0xC0000080

    rdmsr

    or eax, 1

    wrmsr

    ;
    ; IA32_STAR
    ;
    ; SYSCALL:
    ;   CS = 0x18
    ;   SS = 0x20
    ;
    ; SYSRET:
    ;   CS = 0x20
    ;   SS = 0x28
    ;
    mov ecx, 0xC0000081

    xor eax, eax
    mov edx, 0x00100018

    wrmsr

    ;
    ; IA32_LSTAR
    ;
    mov ecx, 0xC0000082

    mov rax, user_syscall_entry
    mov rdx, rax
    shr rdx, 32

    wrmsr

    ;
    ; IA32_FMASK
    ;
    mov ecx, 0xC0000084

    mov eax, 0x00000200
    xor edx, edx

    wrmsr

    mov byte [gdt_ready], 1

.ready:
    ret


user_exit_to_kernel:
    cli

    mov rax, [user_kernel_cr3]
    mov cr3, rax

    mov rsp, [user_kernel_saved_rsp]

    jmp [user_kernel_return]


user_syscall_entry:
    cli

    ;
    ; SYSCALL places:
    ;   RCX = user RIP
    ;   R11 = user RFLAGS
    ;   RSP = user stack
    ;
    mov [user_syscall_user_rsp], rsp

    ;
    ; Switch to kernel stack.
    ;
    mov rsp, [user_kernel_stack_top]

    ;
    ; Save registers.
    ;
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8

    push rdi
    push rsi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, rsp

    call syscall_dispatch

    ;
    ; Restore registers.
    ;
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rbp
    pop rsi
    pop rdi

    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    mov rsp, [user_syscall_user_rsp]

    sti

    sysret


section .data

align 8

gdt_start:

    ;
    ; 0x00: null
    ;
    dq 0

    ;
    ; 0x08: 32-bit kernel code
    ;
    dq 0x00CF9A000000FFFF

    ;
    ; 0x10: kernel data
    ;
    dq 0x00CF92000000FFFF

    ;
    ; 0x18: 64-bit kernel code
    ;
    dq 0x00AF9A000000FFFF

    ;
    ; 0x20: Ring 3 64-bit code
    ;
    dq 0x00AFFA000000FFFF

    ;
    ; 0x28: Ring 3 data
    ;
    dq 0x00CFF2000000FFFF

gdt_tss:

    times 16 db 0

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dq gdt_start


section .bss

align 16

tss:
    resb 104

align 8

gdt_ready:
    resb 1

user_target_cr3:
    resq 1

user_target_rip:
    resq 1

user_target_rsp:
    resq 1

user_syscall_user_rsp:
    resq 1
