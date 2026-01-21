#include "network_driver.h"
#include <stddef.h>

static network_driver_t* drivers[MAX_INTERFACES];
static int driver_count = 0;

void register_network_driver(network_driver_t* driver) {
    if (driver_count < MAX_INTERFACES) {
        drivers[driver_count++] = driver;
    }
}

void init_network_drivers(void) {
    // Initialize specific hardware drivers
    rtl8139_init();
    
    // More drivers can be added here
}
