#include "kernel.h"
#include "process.h"

// Central Interrupt Dispatcher
void handle_interrupt(interrupt_frame_t* frame) {
    uint64_t int_no = frame->int_no;
    
    switch (int_no) {
        case 32: // Timer Interrupt (IRQ 0)
            timer_handler(frame);
            break;
        case 128: // System Call (int 0x80)
            handle_syscall(frame);
            break;
        default:
            // Handle other interrupts (keyboard, mouse, disc)
            // driver_dispatch_interrupt(int_no, frame);
            break;
    }
    
    // Acknowledgement for PIC/APIC
    if (int_no >= 32 && int_no <= 47) {
        if (int_no >= 40) outb(0xA0, 0x20); // Slave
        outb(0x20, 0x20); // Master
    }
}
