#include "../network_driver.h"
#include "../../kernel/kernel.h"
#include "../../kernel/memory.h"
#include "../../kernel/io.h"
#include "../../kernel/interrupt.h"
#include <string.h>

// RTL8139 Registers
#define RTL_REG_MAC0 0x00
#define RTL_REG_MAR0 0x08
#define RTL_REG_RBSTART 0x30
#define RTL_REG_COMMAND 0x37
#define RTL_REG_CAPR 0x38
#define RTL_REG_IMR 0x3C
#define RTL_REG_ISR 0x3E
#define RTL_REG_TCR 0x40
#define RTL_REG_RCR 0x44
#define RTL_REG_CONFIG1 0x52

// Command register bits
#define RTL_CMD_RESET 0x10
#define RTL_CMD_RECV_ENABLE 0x08
#define RTL_CMD_XMIT_ENABLE 0x04
#define RTL_CMD_BUFFER_EMPTY 0x01

static uint16_t io_base;
static uint8_t mac[6];
static uint8_t* rx_buffer;
static uint32_t rx_offset = 0;

void rtl8139_handler(void* rsp) {
    uint16_t isr = inw(io_base + RTL_REG_ISR);
    outw(io_base + RTL_REG_ISR, isr); // Acknowledge interrupts

    if (isr & 0x01) { // ROK (Receive OK)
        while (!(inb(io_base + RTL_REG_COMMAND) & RTL_CMD_BUFFER_EMPTY)) {
            uint16_t header = *(uint16_t*)(rx_buffer + rx_offset);
            uint16_t len = *(uint16_t*)(rx_buffer + rx_offset + 2);
            
            if (header & 0x01) { // Packet OK
                // Pass to network stack (placeholder)
                // handle_ethernet_packet(rx_buffer + rx_offset + 4, len - 4);
            }
            
            rx_offset = (rx_offset + len + 4 + 3) & ~3;
            rx_offset %= 8192;
            outw(io_base + RTL_REG_CAPR, rx_offset - 16);
        }
    }
}

void rtl8139_init(void) {
    // In a real OS, we would find the device via PCI scan
    io_base = 0xC000; // Example I/O base
    
    // 1. Power on
    outb(io_base + RTL_REG_CONFIG1, 0x00);
    
    // 2. Software reset
    outb(io_base + RTL_REG_COMMAND, RTL_CMD_RESET);
    while (inb(io_base + RTL_REG_COMMAND) & RTL_CMD_RESET);
    
    // 3. Initialize RX buffer
    rx_buffer = (uint8_t*)kmalloc(8192 + 16 + 1500);
    outl(io_base + RTL_REG_RBSTART, (uint32_t)(uint64_t)rx_buffer);
    
    // 4. Set IMR (Interrupt Mask Register)
    outw(io_base + RTL_REG_IMR, 0x0005); // ROK and TOK
    
    // 5. Configure RCR (Receive Configuration Register)
    outl(io_base + RTL_REG_RCR, 0x0000000F); // Accept Broadcast, Multicast, My Physical, All Physical
    
    // 6. Enable RX and TX
    outb(io_base + RTL_REG_COMMAND, RTL_CMD_RECV_ENABLE | RTL_CMD_XMIT_ENABLE);
    
    // 7. Get MAC address
    for (int i = 0; i < 6; i++) {
        mac[i] = inb(io_base + RTL_REG_MAC0 + i);
    }
    
    kprintf("RTL8139 initialized. MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
