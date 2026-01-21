#include "../network.h"
#include <string.h>

// IPv4 Protocol Implementation
void ip_handle_packet(void* data, size_t len) {
    if (len < sizeof(ip_header_t)) return;
    
    ip_header_t* ip = (ip_header_t*)data;
    if (ip->version != 4) return;
    
    // Checksum verification (simplified)
    // if (ip_checksum(ip) != 0) return;
    
    void* payload = (uint8_t*)data + (ip->header_len * 4);
    size_t payload_len = ntohs(ip->total_len) - (ip->header_len * 4);
    
    switch (ip->protocol) {
        case IP_PROTO_ICMP:
            icmp_handle_packet(ip, payload, payload_len);
            break;
        case IP_PROTO_TCP:
            tcp_handle_packet(ip, payload, payload_len);
            break;
        case IP_PROTO_UDP:
            udp_handle_packet(ip, payload, payload_len);
            break;
    }
}

void ip_send_packet(uint32_t dest_ip, uint8_t protocol, void* data, size_t len) {
    // Routing decision
    // uint8_t* dest_mac = arp_lookup(dest_ip);
    // if (!dest_mac) { arp_request(dest_ip); return; }
    
    // Construct IP header and send via Ethernet
}
