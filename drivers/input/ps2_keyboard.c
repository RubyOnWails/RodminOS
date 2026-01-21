#include "input.h"
#include "../kernel/io.h"
#include "../kernel/interrupt.h"
#include "../kernel/kernel.h"

#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

static void keyboard_interrupt_handler(interrupt_frame_t* frame) {
    uint8_t scancode = inb(PS2_DATA_PORT);
    
    input_event_t event;
    event.type = INPUT_KEYBOARD;
    event.data.keyboard.scancode = scancode;
    event.data.keyboard.pressed = !(scancode & 0x80);
    event.data.keyboard.keycode = scancode & 0x7F;
    
    handle_input_event(&event);
}

void ps2_keyboard_init(void) {
    // Register interrupt handler for IRQ 1 (INT 33)
    register_interrupt_handler(33, keyboard_interrupt_handler);
    
    // Enable keyboard
    while(inb(PS2_STATUS_PORT) & 0x02);
    outb(PS2_COMMAND_PORT, 0xAE);
    
    kprintf("PS/2 Keyboard initialized\n");
}
