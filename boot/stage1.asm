; Rodmin OS Stage 1 Bootloader
; 64-bit bootloader with secure boot and multi-boot support

[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Display boot message
    mov si, boot_msg
    call print_string

    ; Load stage 2 bootloader
    mov ah, 0x02        ; Read sectors
    mov al, 8           ; Number of sectors to read
    mov ch, 0           ; Cylinder
    mov cl, 2           ; Sector (starts from 1, we're at 1, so load from 2)
    mov dh, 0           ; Head
    mov dl, 0x80        ; Drive (0x80 = first hard disk)
    mov bx, 0x1000      ; Load to 0x1000
    int 0x13

    jc disk_error

    ; Verify stage 2 signature
    cmp word [0x1000], 0xAA55
    jne invalid_stage2

    ; Jump to stage 2
    jmp 0x1000

disk_error:
    mov si, disk_error_msg
    call print_string
    jmp halt

invalid_stage2:
    mov si, invalid_msg
    call print_string
    jmp halt

print_string:
    mov ah, 0x0E
.loop:
    lodsb
    cmp al, 0
    je .done
    int 0x10
    jmp .loop
.done:
    ret

halt:
    hlt
    jmp halt

boot_msg db 'Rodmin OS Bootloader v1.0', 0x0D, 0x0A, 'Loading Stage 2...', 0x0D, 0x0A, 0
disk_error_msg db 'Disk read error!', 0x0D, 0x0A, 0
invalid_msg db 'Invalid Stage 2!', 0x0D, 0x0A, 0

times 510-($-$$) db 0
dw 0xAA55