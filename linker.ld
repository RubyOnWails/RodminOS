/* Rodmin OS Kernel Linker Script */

ENTRY(_start)

SECTIONS
{
    /* Kernel loaded at 1MB */
    . = 0x100000;
    
    /* Code section */
    .text ALIGN(4K) : {
        *(.multiboot)
        *(.text)
        *(.text.*)
    }
    
    /* Read-only data */
    .rodata ALIGN(4K) : {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Initialized data */
    .data ALIGN(4K) : {
        *(.data)
        *(.data.*)
    }
    
    /* Uninitialized data */
    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }
    
    /* Stack */
    .stack ALIGN(4K) : {
        . += 0x4000; /* 16KB stack */
    }
    
    /* Kernel end marker */
    kernel_end = .;
}