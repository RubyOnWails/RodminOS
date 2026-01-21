#include "input.h"
#include "../../gui/gui.h"
#include "../../kernel/kernel.h"

void handle_input_event(input_event_t* event) {
    if(event->type == INPUT_KEYBOARD) {
        // Pass to GUI or shell
        gui_handle_keyboard(&event->data.keyboard);
    } else if(event->type == INPUT_MOUSE) {
        // Pass to GUI
        gui_handle_mouse(&event->data.mouse);
    }
}

void input_init(void) {
    ps2_keyboard_init();
    ps2_mouse_init();
}
