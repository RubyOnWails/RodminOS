#include "network.h"
#include <string.h>

void handle_arp_packet(void* packet) {
    arp_packet_t* arp = (arp_packet_t*)packet;
    
    if (ntohs(arp->operation) == 1) { // ARP Request
        // In a real OS, we would check if target_ip is ours and send a reply
        kprintf("ARP Request for %u.%u.%u.%u from %u.%u.%u.%u\n",
                (arp->target_ip >> 24) & 0xFF, (arp->target_ip >> 16) & 0xFF,
                (arp->target_ip >> 8) & 0xFF, arp->target_ip & 0xFF,
                (arp->sender_ip >> 24) & 0xFF, (arp->sender_ip >> 16) & 0xFF,
                (arp->sender_ip >> 8) & 0xFF, arp->sender_ip & 0xFF);
    }
}

void handle_icmp_packet(ip_packet_t* ip_packet) {
    icmp_packet_t* icmp = (icmp_packet_t*)ip_packet->data;
    
    if (icmp->type == 8) { // Echo Request
        kprintf("ICMP Echo Request received from %u.%u.%u.%u\n",
                (ip_packet->src_addr >> 24) & 0xFF, (ip_packet->src_addr >> 16) & 0xFF,
                (ip_packet->src_addr >> 8) & 0xFF, ip_packet->src_addr & 0xFF);
        // send_icmp_reply(...)
    }
}

void handle_tcp_data(tcp_connection_t* conn, tcp_packet_t* packet) {
    // Process incoming TCP data
    size_t len = packet->data_len;
    if (len > 0) {
        // Append to connection receive buffer
        // wake_up_reader(conn);
    }
}
