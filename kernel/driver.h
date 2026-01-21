#ifndef DRIVER_H
#define DRIVER_H

#include "kernel.h"

void driver_init(void);
void register_driver(driver_t* driver);
driver_t* find_driver(const char* name);

#endif
