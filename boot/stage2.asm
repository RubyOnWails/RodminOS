; Rodmin OS Stage 2 Bootloader
; Handles 64-bit transition, memory detection, and kernel loading

[BITS 16]
[ORG 0x1000]

dw 0xAA55  ; Stage 2 signature

stage2_start:
    cli
    
    ; Enable A20 line
    call enable_a20
    
    ; Get memory map
    call detect_memory
    
    ; Load kernel
    call load_kernel
    
    ; Setup GDT
    lgdt [gdt_descriptor]
    
    ; Enter protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp CODE_SEG:protected_mode

[BITS 32]
protected_mode:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Setup paging for long mode
    call setup_paging
    
    ; Enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr
    
    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    jmp CODE_SEG_64:long_mode

[BITS 64]
long_mode:
    ; Setup 64-bit segments
    mov ax, DATA_SEG_64
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Jump to kernel
    mov rax, 0x100000
    jmp rax

[BITS 16]
enable_a20:
    ; Fast A20 gate method
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

detect_memory:
    ; Use INT 15h, EAX=E820h to detect memory
    mov di, 0x8000      ; Memory map destination
    xor ebx, ebx
    mov edx, 0x534D4150 ; 'SMAP'
    
.loop:
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    
    jc .done
    cmp eax, 0x534D4150
    jne .done
    
    add di, 24
    test ebx, ebx
    jnz .loop
    
.done:
    ret

load_kernel:
    ; Load kernel from disk
    mov ah, 0x02
    mov al, 64          ; Load 64 sectors (32KB kernel)
    mov ch, 0
    mov cl, 10          ; Start from sector 10
    mov dh, 0
    mov dl, 0x80
    mov bx, 0x0000
    mov es, bx
    mov bx, 0x100000    ; Load kernel to 1MB
    
    ; Convert to LBA addressing for large kernels
    push dx
    push cx
    push bx
    
    mov eax, 9          ; LBA sector 9
    mov ebx, 0x100000   ; Destination
    call read_lba
    
    pop bx
    pop cx
    pop dx
    ret

read_lba:
    ; Read using LBA addressing
    push eax
    push ebx
    
    ; Setup DAP (Disk Address Packet)
    mov word [dap_size], 0x10
    mov word [dap_sectors], 64
    mov word [dap_offset], 0x0000
    mov word [dap_segment], 0x1000
    mov dword [dap_lba_low], eax
    mov dword [dap_lba_high], 0
    
    mov ah, 0x42
    mov dl, 0x80
    mov si, dap_size
    int 0x13
    
    pop ebx
    pop eax
    ret

setup_paging:
    ; Setup identity paging for first 2MB
    mov edi, 0x70000    ; Page tables at 0x70000
    
    ; Clear page tables
    mov ecx, 0x3000
    xor eax, eax
    rep stosd
    
    mov edi, 0x70000
    
    ; PML4[0] -> PDPT
    mov dword [edi], 0x71000 | 3
    mov dword [edi + 4], 0
    
    ; PDPT[0] -> PD
    mov dword [0x71000], 0x72000 | 3
    mov dword [0x71004], 0
    
    ; PD entries (2MB pages)
    mov edi, 0x72000
    mov eax, 0x83       ; Present, writable, 2MB page
    mov ecx, 512
    
.setup_pd:
    mov [edi], eax
    add eax, 0x200000   ; Next 2MB
    add edi, 8
    loop .setup_pd
    
    ; Load CR3
    mov eax, 0x70000
    mov cr3, eax
    ret

; GDT
gdt_start:
    dq 0                ; Null descriptor

gdt_code_32:
    dw 0xFFFF           ; Limit low
    dw 0x0000           ; Base low
    db 0x00             ; Base middle
    db 10011010b        ; Access byte
    db 11001111b        ; Granularity
    db 0x00             ; Base high

gdt_data_32:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_code_64:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 10101111b        ; 64-bit code segment
    db 0x00

gdt_data_64:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 10101111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code_32 - gdt_start
DATA_SEG equ gdt_data_32 - gdt_start
CODE_SEG_64 equ gdt_code_64 - gdt_start
DATA_SEG_64 equ gdt_data_64 - gdt_start

; Disk Address Packet
dap_size: dw 0
dap_reserved: dw 0
dap_sectors: dw 0
dap_offset: dw 0
dap_segment: dw 0
dap_lba_low: dd 0
dap_lba_high: dd 0

times 4096-($-$$) db 0