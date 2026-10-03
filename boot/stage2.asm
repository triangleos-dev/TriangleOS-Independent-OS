bits 16
org 0x8000

start:
    cli

    mov [boot_drive], dl

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7000


    ; ==========================================
    ; Stage 2 message
    ; ==========================================

    mov si, message

.print:
    lodsb
    test al, al
    jz memory_map

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp .print


; ==========================================
; BIOS E820 memory map
;
; Count:   0000:4FF0
; Entries: 0000:5000
; Maximum: 128 entries
; ==========================================

memory_map:

    xor ax, ax
    mov ds, ax
    mov es, ax

    mov word [0x4FF0], 0

    mov di, 0x5000
    xor ebx, ebx


e820_next:

    cmp word [0x4FF0], 128
    jae e820_done

    mov eax, 0xE820
    mov edx, 0x534D4150       ; "SMAP"
    mov ecx, 24

    ; ACPI 3.0 attributes
    mov dword [es:di + 20], 1

    push es
    push di

    int 0x15

    pop di
    pop es

    jc e820_error

    cmp eax, 0x534D4150
    jne e820_error

    cmp ecx, 20
    jb e820_error

    inc word [0x4FF0]

    add di, 24

    test ebx, ebx
    jnz e820_next


e820_done:

    ; ==========================================
    ; Load kernel
    ;
    ; LBA 9
    ; Destination 0x20000
    ; 64 sectors = 32 KiB
    ; ==========================================

    xor ax, ax
    mov ds, ax

    mov dl, [boot_drive]
    mov si, kernel_dap

    mov ah, 0x42
    int 0x13

    jc disk_error


    ; ==========================================
    ; Enable A20
    ; ==========================================

    in al, 0x92
    or al, 00000010b
    out 0x92, al


    ; ==========================================
    ; Load GDT
    ; ==========================================

    xor ax, ax
    mov ds, ax

    lgdt [gdt_descriptor]


    ; ==========================================
    ; Enter protected mode
    ; ==========================================

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode


; =================================================
; 32-bit protected mode
; =================================================

bits 32

protected_mode:

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000


    ; ==========================================
    ; Create 4 GiB identity-mapped page tables
    ;
    ; 0x9000 = PML4
    ; 0xA000 = PDPT
    ; 0xB000 = PD 0
    ; 0xC000 = PD 1
    ; 0xD000 = PD 2
    ; 0xE000 = PD 3
    ; ==========================================

    mov edi, 0x9000
    mov ecx, 1280
    xor eax, eax
    rep stosd


    ; PML4[0] -> PDPT
    mov dword [0x9000], 0xA003


    ; PDPT entries

    mov dword [0xA000], 0xB003
    mov dword [0xA008], 0xC003
    mov dword [0xA010], 0xD003
    mov dword [0xA018], 0xE003


    ; ==========================================
    ; Fill 4 page directories
    ;
    ; 512 entries × 2 MiB
    ; = 1 GiB per directory
    ;
    ; 4 directories = 4 GiB
    ; ==========================================

    mov edi, 0xB000
    mov eax, 0x00000083
    mov ecx, 2048


fill_page_directories:

    mov dword [edi], eax

    add eax, 0x00200000
    add edi, 8

    loop fill_page_directories


    ; ==========================================
    ; Load PML4
    ; ==========================================

    mov eax, 0x9000
    mov cr3, eax


    ; ==========================================
    ; Enable PAE
    ; ==========================================

    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax


    ; ==========================================
    ; Enable Long Mode
    ; ==========================================

    mov ecx, 0xC0000080
    rdmsr

    or eax, (1 << 8)

    wrmsr


    ; ==========================================
    ; Enable paging
    ; ==========================================

    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax


    ; ==========================================
    ; Enter 64-bit mode
    ; ==========================================

    jmp 0x18:long_mode


; =================================================
; 64-bit long mode
; =================================================

bits 64

long_mode:

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    mov rsp, 0x90000

    ; ==========================================
    ; Copy kernel
    ;
    ; 0x20000 -> 0x100000
    ; 32 KiB
    ; ==========================================

    cld

    mov rsi, 0x20000
    mov rdi, 0x100000

    mov rcx, 4096
    rep movsq


    ; ==========================================
    ; Jump to kernel
    ; ==========================================

    mov rax, 0x100000
    call rax


.hang:
    cli
    hlt
    jmp .hang


; =================================================
; E820 error
; =================================================

bits 16

e820_error:

    mov si, e820_error_message

e820_print:
    lodsb
    test al, al
    jz e820_halt

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp e820_print

e820_halt:
    cli
    hlt
    jmp e820_halt


; =================================================
; Disk error
; =================================================

disk_error:

    mov si, disk_error_message

disk_print:
    lodsb
    test al, al
    jz disk_halt

    mov ah, 0x0E
    mov bh, 0
    int 0x10

    jmp disk_print

disk_halt:
    cli
    hlt
    jmp disk_halt


; =================================================
; Messages
; =================================================

message:
    db "Stage 2 reached!", 13, 10, 0

e820_error_message:
    db "E820 memory detection failed!", 13, 10, 0

disk_error_message:
    db "Kernel disk read failed!", 13, 10, 0


boot_drive:
    db 0


; =================================================
; Kernel Disk Address Packet
; =================================================

kernel_dap:
    db 0x10
    db 0x00

    dw 64
    dw 0x0000
    dw 0x2000

    dq 9


; =================================================
; GDT
; =================================================

gdt_start:

gdt_null:
    dq 0

; 0x08 = 32-bit code
gdt_code32:
    dq 0x00CF9A000000FFFF

; 0x10 = data
gdt_data:
    dq 0x00CF92000000FFFF

; 0x18 = 64-bit code
gdt_code64:
    dq 0x00AF9A000000FFFF

gdt_end:


gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start
