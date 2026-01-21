#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    INPUT_KEYBOARD,
    INPUT_MOUSE
} input_type_t;

typedef struct {
    uint32_t scancode;
    uint32_t keycode;
    bool pressed;
} keyboard_event_t;

typedef struct {
    int32_t dx;
    int32_t dy;
    uint32_t buttons;
} mouse_event_t;

typedef struct {
    input_type_t type;
    union {
        keyboard_event_t keyboard;
        mouse_event_t mouse;
    } data;
} input_event_t;

void input_init(void);
void handle_input_event(input_event_t* event);

#endif
