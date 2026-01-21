#include "interrupt.h"
#include "kernel.h"
#include "io.h"

static interrupt_handler_t handlers[MAX_INTERRUPTS];

void interrupt_init(void) {
    // Initialize IDT and PIC remapping logic here
    // This is a placeholder for the actual IDT setup
    for(int i = 0; i < MAX_INTERRUPTS; i++) {
        handlers[i] = NULL;
    }
    
    kprintf("Interrupt system initialized\n");
}

void register_interrupt_handler(uint8_t n, interrupt_handler_t handler) {
    handlers[n] = handler;
}

void handle_interrupt(uint8_t n, interrupt_frame_t* frame) {
    if(handlers[n]) {
        handlers[n](frame);
    } else {
        kprintf("Unhandled interrupt: %d\n", n);
    }
}
