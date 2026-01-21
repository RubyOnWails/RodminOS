#include "../kernel/kernel.h"
#include "../kernel/io.h"

// Intel HDA / AC97 Registers
#define AC97_RESET 0x00
#define AC97_MASTER_VOLUME 0x02
#define AC97_PCM_OUT_VOLUME 0x18

static uint16_t audio_io_base;

void audio_init(uint16_t io_base) {
    audio_io_base = io_base;
    
    // Reset codec
    outw(audio_io_base + AC97_RESET, 0xFFFF);
    
    // Set volumes
    outw(audio_io_base + AC97_MASTER_VOLUME, 0x0000); // Max volume
    outw(audio_io_base + AC97_PCM_OUT_VOLUME, 0x0000);
    
    kprintf("Audio Driver (AC97) initialized at 0x%x\n", io_base);
}

void audio_play_beep(void) {
    // Basic PC speaker beep as fallback
    uint32_t div = 1193180 / 440;
    outb(0x43, 0xB6);
    outb(0x42, (uint8_t)(div));
    outb(0x42, (uint8_t)(div >> 8));
    
    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3)) {
        outb(0x61, tmp | 3);
    }
}
