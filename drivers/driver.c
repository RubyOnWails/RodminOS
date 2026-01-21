#include "../kernel/driver.h"
#include <string.h>

static driver_t* driver_list = NULL;

void driver_init(void) {
    // Initialize specific hardware drivers
    // cpu_init_drivers();
    // pci_init_drivers();
    
    kprintf("Driver system initialized\n");
}

void register_driver(driver_t* driver) {
    driver->next = driver_list;
    driver_list = driver;
    
    if(driver->init) {
        driver->init();
    }
}

driver_t* find_driver(const char* name) {
    driver_t* current = driver_list;
    while(current) {
        if(strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}
