#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

#define MAX_INTERRUPTS 256

typedef struct {
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __attribute__((packed)) interrupt_frame_t;

typedef void (*interrupt_handler_t)(interrupt_frame_t* frame);

void interrupt_init(void);
void register_interrupt_handler(uint8_t n, interrupt_handler_t handler);
void enable_interrupts(void);
void disable_interrupts(void);

#endif
