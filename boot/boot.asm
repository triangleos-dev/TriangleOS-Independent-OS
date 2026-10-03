bits 16
org 0x7C00

start:
    cli

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    mov si, message

.print:
    lodsb
    test al, al
    jz .load_stage2

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp .print


.load_stage2:

    mov dl, [boot_drive]
    mov si, dap

    mov ah, 0x42
    int 0x13

    jc disk_error

    ; Stage 2 is loaded at physical 0x8000.
    jmp 0x0000:0x8000


disk_error:

    mov si, error_message

.error_print:
    lodsb
    test al, al
    jz .halt

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp .error_print

.halt:
    cli
    hlt
    jmp .halt


boot_drive:
    db 0


message:
    db "TriangleOS stage 1", 13, 10, 0

error_message:
    db "Disk read failed!", 13, 10, 0


dap:
    db 0x10
    db 0x00

    dw 8
    dw 0x8000
    dw 0x0000

    dq 1


times 510-($-$$) db 0
dw 0xAA55
