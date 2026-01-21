#include "input.h"
#include "../kernel/io.h"
#include "../kernel/interrupt.h"
#include "../kernel/kernel.h"

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if(type == 0) {
        while(timeout--) {
            if((inb(PS2_STATUS_PORT) & 1) == 1) return;
        }
    } else {
        while(timeout--) {
            if((inb(PS2_STATUS_PORT) & 2) == 0) return;
        }
    }
}

static void mouse_write(uint8_t a_write) {
    mouse_wait(1);
    outb(PS2_STATUS_PORT, 0xD4);
    mouse_wait(1);
    outb(PS2_DATA_PORT, a_write);
}

static void mouse_interrupt_handler(interrupt_frame_t* frame) {
    uint8_t status = inb(PS2_STATUS_PORT);
    if(!(status & 0x01) || !(status & 0x20)) return;

    mouse_byte[mouse_cycle++] = inb(PS2_DATA_PORT);

    if(mouse_cycle == 3) {
        mouse_cycle = 0;
        
        input_event_t event;
        event.type = INPUT_MOUSE;
        event.data.mouse.buttons = mouse_byte[0] & 0x07;
        event.data.mouse.dx = mouse_byte[1];
        event.data.mouse.dy = mouse_byte[2];
        
        // Handle overflow and sign
        if(mouse_byte[0] & 0x10) event.data.mouse.dx -= 256;
        if(mouse_byte[0] & 0x20) event.data.mouse.dy -= 256;
        
        handle_input_event(&event);
    }
}

void ps2_mouse_init(void) {
    // Enable mouse in PS/2 controller
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, 0xA8);
    
    // Enable interrupts
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, 0x20);
    mouse_wait(0);
    uint8_t status = inb(PS2_DATA_PORT) | 2;
    mouse_wait(1);
    outb(PS2_COMMAND_PORT, 0x60);
    mouse_wait(1);
    outb(PS2_DATA_PORT, status);
    
    // Set default settings
    mouse_write(0xF6);
    inb(PS2_DATA_PORT);
    
    // Enable packet streaming
    mouse_write(0xF4);
    inb(PS2_DATA_PORT);
    
    // Register interrupt handler for IRQ 12 (INT 44)
    register_interrupt_handler(44, mouse_interrupt_handler);
    
    kprintf("PS/2 Mouse initialized\n");
}
