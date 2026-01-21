#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>

// Network constants
#define MAX_INTERFACES 16
#define MAX_ROUTES 256
#define MAX_ARP_ENTRIES 256
#define MAX_SOCKETS 1024
#define MAX_TCP_CONNECTIONS 512
#define SOCKET_BUFFER_SIZE 65536
#define TCP_MSS 1460

// Protocol numbers
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

// Ethernet types
#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806

// Socket types
#define SOCK_STREAM 1
#define SOCK_DGRAM 2

// Address families
#define AF_INET 2

// Socket states
#define SOCKET_CREATED 0
#define SOCKET_BOUND 1
#define SOCKET_LISTENING 2
#define SOCKET_CONNECTED 3
#define SOCKET_CLOSED 4

// TCP states
#define TCP_CLOSED 0
#define TCP_LISTEN 1
#define TCP_SYN_SENT 2
#define TCP_SYN_RECEIVED 3
#define TCP_ESTABLISHED 4
#define TCP_FIN_WAIT_1 5
#define TCP_FIN_WAIT_2 6
#define TCP_CLOSE_WAIT 7
#define TCP_CLOSING 8
#define TCP_LAST_ACK 9
#define TCP_TIME_WAIT 10

// TCP flags
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

// Interface flags
#define IFF_UP 0x01
#define IFF_RUNNING 0x02
#define IFF_LOOPBACK 0x04

// Special addresses
#define IP_LOOPBACK 0x7F000001
#define IP_BROADCAST 0xFFFFFFFF

// Ethernet frame structure
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
    uint8_t data[];
} __attribute__((packed)) ethernet_frame_t;

// IP packet structure
typedef struct {
    uint8_t version:4;
    uint8_t header_len:4;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dest_addr;
    uint8_t data[];
} __attribute__((packed)) ip_packet_t;

typedef struct {
    uint8_t version:4;
    uint8_t header_len:4;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dest_addr;
} __attribute__((packed)) ip_header_t;

// TCP packet structure
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t header_len:4;
    uint8_t reserved:4;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
    uint8_t data[];
} __attribute__((packed)) tcp_packet_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t header_len:4;
    uint8_t reserved:4;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed)) tcp_header_t;

// UDP packet structure
typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
    uint8_t data[];
} __attribute__((packed)) udp_packet_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

// ICMP packet structure
typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint32_t data;
    uint8_t payload[];
} __attribute__((packed)) icmp_packet_t;

// ARP packet structure
typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_len;
    uint8_t protocol_len;
    uint16_t operation;
    uint8_t sender_mac[6];
    uint32_t sender_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) arp_packet_t;

// Network interface
typedef struct {
    char name[16];
    uint32_t addr;
    uint32_t netmask;
    uint32_t broadcast;
    uint8_t mac_addr[6];
    uint32_t mtu;
    uint32_t flags;
} network_interface_t;

// Routing table entry
typedef struct {
    uint32_t dest;
    uint32_t netmask;
    uint32_t gateway;
    uint32_t interface;
    uint32_t metric;
} route_entry_t;

// ARP table entry
typedef struct {
    uint32_t ip_addr;
    uint8_t mac_addr[6];
    uint64_t timestamp;
    bool permanent;
} arp_entry_t;

// TCP connection
typedef struct {
    uint32_t local_addr;
    uint16_t local_port;
    uint32_t remote_addr;
    uint16_t remote_port;
    uint32_t state;
    uint32_t seq_num;
    uint32_t ack_num;
    uint16_t window_size;
    uint8_t* recv_buffer;
    uint8_t* send_buffer;
    uint32_t recv_head, recv_tail;
    uint32_t send_head, send_tail;
} tcp_connection_t;

// Socket structure
typedef struct {
    int domain;
    int type;
    int protocol;
    uint32_t state;
    
    uint32_t local_addr;
    uint16_t local_port;
    uint32_t remote_addr;
    uint16_t remote_port;
    
    uint8_t* recv_buffer;
    uint8_t* send_buffer;
    uint32_t recv_head, recv_tail;
    uint32_t send_head, send_tail;
    
    int backlog;
    tcp_connection_t* tcp_connection;
} socket_t;

// Protocol handler
typedef struct protocol_handler {
    uint16_t protocol;
    void (*func)(void* packet);
    struct protocol_handler* next;
} protocol_handler_t;

// Network context
typedef struct {
    network_interface_t interfaces[MAX_INTERFACES];
    uint32_t interface_count;
    
    route_entry_t routes[MAX_ROUTES];
    uint32_t route_count;
    
    arp_entry_t arp_table[MAX_ARP_ENTRIES];
    uint32_t arp_count;
    
    tcp_connection_t tcp_connections[MAX_TCP_CONNECTIONS];
    
    uint16_t ip_id_counter;
    uint16_t tcp_port_counter;
    uint16_t udp_port_counter;
    
    bool ip_forwarding;
    
    protocol_handler_t* protocol_handlers;
} network_context_t;

// Socket address structures
struct sockaddr {
    uint16_t sa_family;
    char sa_data[14];
};

struct sockaddr_in {
    uint16_t sin_family;
    uint16_t sin_port;
    struct in_addr {
        uint32_t s_addr;
    } sin_addr;
    char sin_zero[8];
};

typedef uint32_t socklen_t;
typedef int64_t ssize_t;

// Function prototypes

// Network initialization
void network_init(void);
void ethernet_init(void);
void ip_init(void);
void tcp_init(void);
void udp_init(void);

// Socket API
int socket(int domain, int type, int protocol);
int bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
int connect(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
ssize_t send(int sockfd, const void* buf, size_t len, int flags);
ssize_t recv(int sockfd, void* buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void* buf, size_t len, int flags,
               const struct sockaddr* dest_addr, socklen_t addrlen);
ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags,
                 struct sockaddr* src_addr, socklen_t* addrlen);
int close_socket(int sockfd);

// TCP implementation
int tcp_connect(socket_t* sock);
ssize_t tcp_send(socket_t* sock, const void* data, size_t len);
void tcp_close(tcp_connection_t* conn);
void handle_tcp_packet(ip_packet_t* ip_packet);
void tcp_state_machine(tcp_connection_t* conn, tcp_packet_t* packet);
void handle_tcp_data(tcp_connection_t* conn, tcp_packet_t* packet);

// UDP implementation
ssize_t udp_send(socket_t* sock, const void* data, size_t len);
void handle_udp_packet(ip_packet_t* ip_packet);

// IP implementation
int send_ip_packet(uint32_t src_addr, uint32_t dest_addr, uint8_t protocol,
                   const void* data, size_t len);
void handle_ip_packet(ethernet_frame_t* frame);
int route_ip_packet(ip_packet_t* packet);
void forward_ip_packet(ip_packet_t* packet);

// ICMP implementation
void handle_icmp_packet(ip_packet_t* ip_packet);
void send_icmp_reply(uint32_t dest_addr, uint8_t type, uint8_t code, const void* data, size_t len);

// ARP implementation
void handle_arp_packet(ethernet_frame_t* frame);
int resolve_mac_address(uint32_t ip_addr, uint8_t* mac_addr);
void send_arp_request(uint32_t target_ip);

// Network interface management
int add_network_interface(const char* name, uint32_t addr, uint32_t netmask, const uint8_t* mac_addr);
void setup_loopback_interface(void);
network_interface_t* find_interface_by_addr(uint32_t addr);
bool is_local_address(uint32_t addr);

// Routing
int add_route(uint32_t dest, uint32_t netmask, uint32_t gateway, uint32_t interface);
route_entry_t* find_route(uint32_t dest_addr);

// Protocol handlers
void register_protocol_handler(uint16_t protocol, void (*handler)(void*));
void register_ip_protocol(uint8_t protocol, void (*handler)(ip_packet_t*));
protocol_handler_t* find_ip_protocol_handler(uint8_t protocol);

// Utility functions
uint16_t calculate_checksum(const void* data, size_t len);
uint16_t calculate_ip_checksum(ip_packet_t* packet);
uint16_t calculate_tcp_checksum(tcp_packet_t* packet, uint32_t src_addr, uint32_t dest_addr);
uint16_t calculate_udp_checksum(udp_packet_t* packet, uint32_t src_addr, uint32_t dest_addr);
int verify_ip_checksum(ip_packet_t* packet);

// Network byte order conversion
uint32_t htonl(uint32_t hostlong);
uint16_t htons(uint16_t hostshort);
uint32_t ntohl(uint32_t netlong);
uint16_t ntohs(uint16_t netshort);

// Socket utilities
socket_t* find_udp_socket(uint16_t port);
tcp_connection_t* find_tcp_connection(uint32_t local_addr, uint16_t local_port,
                                     uint32_t remote_addr, uint16_t remote_port);
tcp_connection_t* allocate_tcp_connection(void);
uint16_t allocate_port(void);
bool is_port_in_use(uint16_t port, int type);
ssize_t socket_recv(socket_t* sock, void* buf, size_t len);
void socket_buffer_write(socket_t* sock, const void* data, size_t len);

// TCP utilities
void build_tcp_packet(tcp_packet_t* packet, tcp_connection_t* conn, uint8_t flags,
                     const void* data, size_t len);
int send_tcp_packet(tcp_packet_t* packet);
void send_tcp_reset(uint32_t dest_addr, uint16_t dest_port, uint32_t src_addr, uint16_t src_port);
uint32_t generate_sequence_number(void);
tcp_connection_t* wait_for_connection(socket_t* listen_sock);
int wait_for_tcp_connection(tcp_connection_t* conn);
void wake_tcp_connection(tcp_connection_t* conn);
void add_to_accept_queue(tcp_connection_t* conn);

// Network drivers
void init_network_drivers(void);

// Standard functions
void memcpy(void* dest, const void* src, size_t n);

#endif