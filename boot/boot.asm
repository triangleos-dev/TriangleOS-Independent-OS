bits 16
org 0x7C00

start:
cli

mov si, message

print:
lodsb
test al, al
jz halt

mov ah, 0x0E
mov bh, 0x00
int 0x10
jmp print

halt:
cli
hlt
jmp halt

message:
db "TriangleOS bootloader started!", 13, 10, 0

times 510-($-$$) db 0
dw 0xAA55
