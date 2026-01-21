#include "../network.h"
#include <string.h>

// TCP State Machine and Segment Handling
void tcp_handle_packet(ip_header_t* ip, void* data, size_t len) {
    if (len < sizeof(tcp_header_t)) return;
    
    tcp_header_t* tcp = (tcp_header_t*)data;
    uint32_t src_ip = ip->src_addr;
    uint16_t src_port = ntohs(tcp->src_port);
    uint16_t dest_port = ntohs(tcp->dest_port);
    
    // Find connection
    // tcp_connection_t* conn = tcp_find_connection(src_ip, src_port, dest_port);
    
    // Handle flags (SYN, ACK, FIN, RST)
    if (tcp->flags & TCP_FLAG_SYN) {
        // Handle connection request
    }
}

void tcp_send_segment(tcp_connection_t* conn, void* data, size_t len, uint8_t flags) {
    // Construct TCP header with sequence/ack numbers
    // send_ip_packet(conn->remote_ip, IP_PROTO_TCP, packet, total_len);
}
