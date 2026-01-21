#include "../network.h"
#include <string.h>

// UDP Datagram Handling
void udp_handle_packet(ip_header_t* ip, void* data, size_t len) {
    if (len < sizeof(udp_header_t)) return;
    
    udp_header_t* udp = (udp_header_t*)data;
    uint16_t dest_port = ntohs(udp->dest_port);
    
    // Dispatch to socket
    // socket_t* sock = socket_find(IP_PROTO_UDP, dest_port);
    // if (sock) { socket_push_data(sock, (uint8_t*)data + sizeof(udp_header_t), ntohs(udp->length) - sizeof(udp_header_t)); }
}

void udp_send_packet(uint32_t dest_ip, uint16_t src_port, uint16_t dest_port, void* data, size_t len) {
    // Construct UDP header and send via IP
}
