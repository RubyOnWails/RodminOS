#include "../network.h"
#include <string.h>

// Ethernet II Frame Handling
void ethernet_send_packet(uint8_t* dest_mac, uint16_t type, void* data, size_t len) {
    uint8_t* buffer = (uint8_t*)kmalloc(len + sizeof(ethernet_header_t));
    ethernet_header_t* eth = (ethernet_header_t*)buffer;
    
    memcpy(eth->dest, dest_mac, 6);
    // get_local_mac(eth->src);
    eth->type = htons(type);
    memcpy(buffer + sizeof(ethernet_header_t), data, len);
    
    // Pass to network driver
    // driver_send(eth_driver, buffer, len + sizeof(ethernet_header_t));
    
    kfree(buffer);
}

void ethernet_handle_packet(void* data, size_t len) {
    if (len < sizeof(ethernet_header_t)) return;
    
    ethernet_header_t* eth = (ethernet_header_t*)data;
    uint16_t type = ntohs(eth->type);
    void* payload = (uint8_t*)data + sizeof(ethernet_header_t);
    size_t payload_len = len - sizeof(ethernet_header_t);
    
    switch (type) {
        case ETHERTYPE_IP:
            ip_handle_packet(payload, payload_len);
            break;
        case ETHERTYPE_ARP:
            arp_handle_packet(payload, payload_len);
            break;
        case ETHERTYPE_IPV6:
            // kprintf("IPv6 packet ignored...\n");
            break;
        default:
            kprintf("Unknown EtherType: 0x%04x\n", type);
            break;
    }
}
