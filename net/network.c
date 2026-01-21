#include "network.h"
#include "../kernel/kernel.h"
#include "../kernel/memory.h"
#include "../drivers/network_driver.h"

// Global networking state
static network_context_t net_ctx;
static socket_t sockets[MAX_SOCKETS];
static bool socket_slots[MAX_SOCKETS];

void network_init(void) {
    // Initialize network context
    net_ctx.interface_count = 0;
    net_ctx.route_count = 0;
    net_ctx.arp_count = 0;
    
    // Initialize socket table
    for(int i = 0; i < MAX_SOCKETS; i++) {
        socket_slots[i] = false;
    }
    
    // Initialize protocol stacks
    ethernet_init();
    ip_init();
    tcp_init();
    udp_init();
    
    // Initialize network drivers
    init_network_drivers();
    
    // Setup loopback interface
    setup_loopback_interface();
    
    kprintf("Network stack initialized\n");
}

void ethernet_init(void) {
    // Initialize Ethernet protocol handler
    register_protocol_handler(ETHERTYPE_IP, handle_ip_packet);
    register_protocol_handler(ETHERTYPE_ARP, handle_arp_packet);
}

void ip_init(void) {
    // Initialize IP protocol
    net_ctx.ip_id_counter = 1;
    
    // Register IP protocol handlers
    register_ip_protocol(IP_PROTO_TCP, handle_tcp_packet);
    register_ip_protocol(IP_PROTO_UDP, handle_udp_packet);
    register_ip_protocol(IP_PROTO_ICMP, handle_icmp_packet);
}

void tcp_init(void) {
    // Initialize TCP state
    net_ctx.tcp_port_counter = 32768;
    
    // Initialize TCP connection table
    for(int i = 0; i < MAX_TCP_CONNECTIONS; i++) {
        net_ctx.tcp_connections[i].state = TCP_CLOSED;
    }
}

void udp_init(void) {
    // Initialize UDP state
    net_ctx.udp_port_counter = 32768;
}

// Socket API implementation

int socket(int domain, int type, int protocol) {
    // Find free socket slot
    int sock_fd = -1;
    for(int i = 0; i < MAX_SOCKETS; i++) {
        if(!socket_slots[i]) {
            sock_fd = i;
            socket_slots[i] = true;
            break;
        }
    }
    
    if(sock_fd == -1) return -1; // No free sockets
    
    socket_t* sock = &sockets[sock_fd];
    
    // Initialize socket
    sock->domain = domain;
    sock->type = type;
    sock->protocol = protocol;
    sock->state = SOCKET_CREATED;
    sock->local_port = 0;
    sock->remote_port = 0;
    sock->local_addr = 0;
    sock->remote_addr = 0;
    
    // Initialize buffers
    sock->recv_buffer = (uint8_t*)kmalloc(SOCKET_BUFFER_SIZE);
    sock->send_buffer = (uint8_t*)kmalloc(SOCKET_BUFFER_SIZE);
    sock->recv_head = 0;
    sock->recv_tail = 0;
    sock->send_head = 0;
    sock->send_tail = 0;
    
    return sock_fd;
}

int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    if(sockfd < 0 || sockfd >= MAX_SOCKETS || !socket_slots[sockfd]) {
        return -1;
    }
    
    socket_t* sock = &sockets[sockfd];
    const struct sockaddr_in* addr_in = (const struct sockaddr_in*)addr;
    
    if(addr_in->sin_family != AF_INET) {
        return -1;
    }
    
    // Check if port is already in use
    uint16_t port = ntohs(addr_in->sin_port);
    if(port != 0 && is_port_in_use(port, sock->type)) {
        return -1;
    }
    
    sock->local_addr = ntohl(addr_in->sin_addr.s_addr);
    sock->local_port = port;
    sock->state = SOCKET_BOUND;
    
    return 0;
}

int listen(int sockfd, int backlog) {
    if(sockfd < 0 || sockfd >= MAX_SOCKETS || !socket_slots[sockfd]) {
        return -1;
    }
    
    socket_t* sock = &sockets[sockfd];
    
    if(sock->type != SOCK_STREAM || sock->state != SOCKET_BOUND) {
        return -1;
    }
    
    sock->backlog = backlog;
    sock->state = SOCKET_LISTENING;
    
    return 0;
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    if(sockfd < 0 || sockfd >= MAX_SOCKETS || !socket_slots[sockfd]) {
        return -1;
    }
    
    socket_t* listen_sock = &sockets[sockfd];
    
    if(listen_sock->state != SOCKET_LISTENING) {
        return -1;
    }
    
    // Wait for incoming connection
    tcp_connection_t* conn = wait_for_connection(listen_sock);
    if(!conn) return -1;
    
    // Create new socket for connection
    int new_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(new_sockfd == -1) return -1;
    
    socket_t* new_sock = &sockets[new_sockfd];
    new_sock->local_addr = listen_sock->local_addr;
    new_sock->local_port = listen_sock->local_port;
    new_sock->remote_addr = conn->remote_addr;
    new_sock->remote_port = conn->remote_port;
    new_sock->state = SOCKET_CONNECTED;
    new_sock->tcp_connection = conn;
    
    // Fill in client address
    if(addr && addrlen) {
        struct sockaddr_in* addr_in = (struct sockaddr_in*)addr;
        addr_in->sin_family = AF_INET;
        addr_in->sin_port = htons(conn->remote_port);
        addr_in->sin_addr.s_addr = htonl(conn->remote_addr);
        *addrlen = sizeof(struct sockaddr_in);
    }
    
    return new_sockfd;
}

int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    if(sockfd < 0 || sockfd >= MAX_SOCKETS || !socket_slots[sockfd]) {
        return -1;
    }
    
    socket_t* sock = &sockets[sockfd];
    const struct sockaddr_in* addr_in = (const struct sockaddr_in*)addr;
    
    if(addr_in->sin_family != AF_INET) {
        return -1;
    }
    
    sock->remote_addr = ntohl(addr_in->sin_addr.s_addr);
    sock->remote_port = ntohs(addr_in->sin_port);
    
    if(sock->type == SOCK_STREAM) {
        // TCP connection
        return tcp_connect(sock);
    } else if(sock->type == SOCK_DGRAM) {
        // UDP - just set remote address
        sock->state = SOCKET_CONNECTED;
        return 0;
    }
    
    return -1;
}

ssize_t send(int sockfd, const void* buf, size_t len, int flags) {
    if(sockfd < 0 || sockfd >= MAX_SOCKETS || !socket_slots[sockfd]) {
        return -1;
    }
    
    socket_t* sock = &sockets[sockfd];
    
    if(sock->type == SOCK_STREAM) {
        return tcp_send(sock, buf, len);
    } else if(sock->type == SOCK_DGRAM) {
        return udp_send(sock, buf, len);
    }
    
    return -1;
}

ssize_t recv(int sockfd, void* buf, size_t len, int flags) {
    if(sockfd < 0 || sockfd >= MAX_SOCKETS || !socket_slots[sockfd]) {
        return -1;
    }
    
    socket_t* sock = &sockets[sockfd];
    
    return socket_recv(sock, buf, len);
}

int close_socket(int sockfd) {
    if(sockfd < 0 || sockfd >= MAX_SOCKETS || !socket_slots[sockfd]) {
        return -1;
    }
    
    socket_t* sock = &sockets[sockfd];
    
    if(sock->type == SOCK_STREAM && sock->tcp_connection) {
        tcp_close(sock->tcp_connection);
    }
    
    // Free buffers
    kfree(sock->recv_buffer);
    kfree(sock->send_buffer);
    
    // Mark socket as free
    socket_slots[sockfd] = false;
    
    return 0;
}

// TCP implementation

int tcp_connect(socket_t* sock) {
    // Allocate local port if not bound
    if(sock->local_port == 0) {
        sock->local_port = allocate_port();
    }
    
    // Find free TCP connection
    tcp_connection_t* conn = allocate_tcp_connection();
    if(!conn) return -1;
    
    // Initialize connection
    conn->local_addr = sock->local_addr;
    conn->local_port = sock->local_port;
    conn->remote_addr = sock->remote_addr;
    conn->remote_port = sock->remote_port;
    conn->state = TCP_SYN_SENT;
    conn->seq_num = generate_sequence_number();
    conn->ack_num = 0;
    
    sock->tcp_connection = conn;
    
    // Send SYN packet
    tcp_packet_t syn_packet;
    build_tcp_packet(&syn_packet, conn, TCP_SYN, NULL, 0);
    send_tcp_packet(&syn_packet);
    
    // Wait for connection to establish
    return wait_for_tcp_connection(conn);
}

ssize_t tcp_send(socket_t* sock, const void* data, size_t len) {
    if(!sock->tcp_connection || sock->tcp_connection->state != TCP_ESTABLISHED) {
        return -1;
    }
    
    tcp_connection_t* conn = sock->tcp_connection;
    
    // Fragment data if necessary
    size_t bytes_sent = 0;
    const uint8_t* ptr = (const uint8_t*)data;
    
    while(bytes_sent < len) {
        size_t chunk_size = (len - bytes_sent > TCP_MSS) ? TCP_MSS : (len - bytes_sent);
        
        tcp_packet_t packet;
        build_tcp_packet(&packet, conn, TCP_PSH | TCP_ACK, ptr + bytes_sent, chunk_size);
        
        if(send_tcp_packet(&packet) != 0) {
            break;
        }
        
        conn->seq_num += chunk_size;
        bytes_sent += chunk_size;
    }
    
    return bytes_sent;
}

void handle_tcp_packet(ip_packet_t* ip_packet) {
    tcp_packet_t* tcp_packet = (tcp_packet_t*)ip_packet->data;
    
    // Find matching connection
    tcp_connection_t* conn = find_tcp_connection(
        ip_packet->dest_addr, ntohs(tcp_packet->dest_port),
        ip_packet->src_addr, ntohs(tcp_packet->src_port)
    );
    
    if(!conn) {
        // No matching connection - send RST
        send_tcp_reset(ip_packet->src_addr, ntohs(tcp_packet->src_port),
                      ip_packet->dest_addr, ntohs(tcp_packet->dest_port));
        return;
    }
    
    // Process packet based on connection state
    tcp_state_machine(conn, tcp_packet);
}

void tcp_state_machine(tcp_connection_t* conn, tcp_packet_t* packet) {
    uint8_t flags = packet->flags;
    
    switch(conn->state) {
        case TCP_LISTEN:
            if(flags & TCP_SYN) {
                // Incoming connection request
                conn->remote_addr = packet->src_addr;
                conn->remote_port = ntohs(packet->src_port);
                conn->ack_num = ntohl(packet->seq_num) + 1;
                conn->seq_num = generate_sequence_number();
                conn->state = TCP_SYN_RECEIVED;
                
                // Send SYN-ACK
                tcp_packet_t syn_ack;
                build_tcp_packet(&syn_ack, conn, TCP_SYN | TCP_ACK, NULL, 0);
                send_tcp_packet(&syn_ack);
            }
            break;
            
        case TCP_SYN_SENT:
            if((flags & (TCP_SYN | TCP_ACK)) == (TCP_SYN | TCP_ACK)) {
                // Connection established
                conn->ack_num = ntohl(packet->seq_num) + 1;
                conn->state = TCP_ESTABLISHED;
                
                // Send ACK
                tcp_packet_t ack;
                build_tcp_packet(&ack, conn, TCP_ACK, NULL, 0);
                send_tcp_packet(&ack);
                
                // Wake up waiting process
                wake_tcp_connection(conn);
            }
            break;
            
        case TCP_SYN_RECEIVED:
            if(flags & TCP_ACK) {
                conn->state = TCP_ESTABLISHED;
                // Add to accept queue
                add_to_accept_queue(conn);
            }
            break;
            
        case TCP_ESTABLISHED:
            if(flags & TCP_FIN) {
                // Remote side closing
                conn->ack_num = ntohl(packet->seq_num) + 1;
                conn->state = TCP_CLOSE_WAIT;
                
                // Send ACK
                tcp_packet_t ack;
                build_tcp_packet(&ack, conn, TCP_ACK, NULL, 0);
                send_tcp_packet(&ack);
            } else if(packet->data_len > 0) {
                // Data packet
                handle_tcp_data(conn, packet);
            }
            break;
            
        case TCP_FIN_WAIT_1:
            if(flags & TCP_ACK) {
                conn->state = TCP_FIN_WAIT_2;
            }
            if(flags & TCP_FIN) {
                conn->ack_num = ntohl(packet->seq_num) + 1;
                conn->state = (conn->state == TCP_FIN_WAIT_2) ? TCP_TIME_WAIT : TCP_CLOSING;
                
                tcp_packet_t ack;
                build_tcp_packet(&ack, conn, TCP_ACK, NULL, 0);
                send_tcp_packet(&ack);
            }
            break;
            
        case TCP_FIN_WAIT_2:
            if(flags & TCP_FIN) {
                conn->ack_num = ntohl(packet->seq_num) + 1;
                conn->state = TCP_TIME_WAIT;
                
                tcp_packet_t ack;
                build_tcp_packet(&ack, conn, TCP_ACK, NULL, 0);
                send_tcp_packet(&ack);
            }
            break;
    }
}

// UDP implementation

ssize_t udp_send(socket_t* sock, const void* data, size_t len) {
    udp_packet_t packet;
    
    packet.src_port = htons(sock->local_port);
    packet.dest_port = htons(sock->remote_port);
    packet.length = htons(sizeof(udp_header_t) + len);
    packet.checksum = 0;
    
    // Copy data
    memcpy(packet.data, data, len);
    
    // Calculate checksum
    packet.checksum = calculate_udp_checksum(&packet, sock->local_addr, sock->remote_addr);
    
    // Send via IP
    return send_ip_packet(sock->local_addr, sock->remote_addr, IP_PROTO_UDP, 
                         &packet, sizeof(udp_header_t) + len);
}

void handle_udp_packet(ip_packet_t* ip_packet) {
    udp_packet_t* udp_packet = (udp_packet_t*)ip_packet->data;
    
    // Find socket listening on destination port
    socket_t* sock = find_udp_socket(ntohs(udp_packet->dest_port));
    if(!sock) return;
    
    // Add data to socket receive buffer
    size_t data_len = ntohs(udp_packet->length) - sizeof(udp_header_t);
    socket_buffer_write(sock, udp_packet->data, data_len);
}

// IP implementation

int send_ip_packet(uint32_t src_addr, uint32_t dest_addr, uint8_t protocol, 
                   const void* data, size_t len) {
    ip_packet_t packet;
    
    // Fill IP header
    packet.version = 4;
    packet.header_len = 5;
    packet.tos = 0;
    packet.total_len = htons(sizeof(ip_header_t) + len);
    packet.id = htons(net_ctx.ip_id_counter++);
    packet.flags_fragment = 0;
    packet.ttl = 64;
    packet.protocol = protocol;
    packet.checksum = 0;
    packet.src_addr = htonl(src_addr);
    packet.dest_addr = htonl(dest_addr);
    
    // Copy data
    memcpy(packet.data, data, len);
    
    // Calculate checksum
    packet.checksum = calculate_ip_checksum(&packet);
    
    // Route packet
    return route_ip_packet(&packet);
}

void handle_ip_packet(ethernet_frame_t* frame) {
    ip_packet_t* packet = (ip_packet_t*)frame->data;
    
    // Verify checksum
    if(verify_ip_checksum(packet) != 0) {
        return; // Invalid checksum
    }
    
    // Check if packet is for us
    uint32_t dest_addr = ntohl(packet->dest_addr);
    if(!is_local_address(dest_addr) && dest_addr != IP_BROADCAST) {
        // Forward packet if routing enabled
        if(net_ctx.ip_forwarding) {
            forward_ip_packet(packet);
        }
        return;
    }
    
    // Handle based on protocol
    protocol_handler_t* handler = find_ip_protocol_handler(packet->protocol);
    if(handler) {
        handler->func(packet);
    }
}

// Network interface management

int add_network_interface(const char* name, uint32_t addr, uint32_t netmask, 
                         const uint8_t* mac_addr) {
    if(net_ctx.interface_count >= MAX_INTERFACES) {
        return -1;
    }
    
    network_interface_t* iface = &net_ctx.interfaces[net_ctx.interface_count];
    
    strncpy(iface->name, name, 15);
    iface->addr = addr;
    iface->netmask = netmask;
    iface->broadcast = addr | (~netmask);
    memcpy(iface->mac_addr, mac_addr, 6);
    iface->mtu = 1500;
    iface->flags = IFF_UP | IFF_RUNNING;
    
    net_ctx.interface_count++;
    
    return 0;
}

void setup_loopback_interface(void) {
    uint8_t loopback_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    add_network_interface("lo", IP_LOOPBACK, 0xFF000000, loopback_mac);
}

// Utility functions

uint16_t calculate_checksum(const void* data, size_t len) {
    const uint16_t* ptr = (const uint16_t*)data;
    uint32_t sum = 0;
    
    // Sum all 16-bit words
    while(len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    
    // Add odd byte if present
    if(len > 0) {
        sum += *(uint8_t*)ptr;
    }
    
    // Fold 32-bit sum to 16 bits
    while(sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0xFF) << 24) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong & 0xFF0000) >> 8) |
           ((hostlong & 0xFF000000) >> 24);
}

uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort & 0xFF00) >> 8);
}

uint32_t ntohl(uint32_t netlong) {
    return htonl(netlong); // Same operation
}

uint16_t ntohs(uint16_t netshort) {
    return htons(netshort); // Same operation
}