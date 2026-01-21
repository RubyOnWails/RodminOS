#ifndef NETWORK_DRIVER_H
#define NETWORK_DRIVER_H

#include <stdint.h>
#include <stddef.h>
#include "../net/network.h"

// Network driver interface
typedef struct {
    char name[32];
    uint8_t mac_addr[6];
    void* private_data;
    
    int (*send)(void* private_data, const void* data, size_t len);
    void (*receive_callback)(void* packet, size_t len);
} network_driver_t;

// Register a network driver
void register_network_driver(network_driver_t* driver);

// Initialize all network drivers
void init_network_drivers(void);

// Hardware specific drivers
void rtl8139_init(void);

#endif
