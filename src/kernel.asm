[BITS 32]
global _start

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    ; enabling A20 line for full memory access
    ; https://wiki.osdev.org/A20_Line
    in al, 0x92
    or al, 2
    out 0x92, al
    jmp $

times 512-($-$$) db 0           ; align to 16 bytes to avoid alignment issues with C objects