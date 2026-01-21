#ifndef SECURITY_H
#define SECURITY_H

#include <stdint.h>
#include <stdbool.h>

void security_init(void);
bool check_permission(uint32_t pid, const char* resource, uint32_t action);

#endif
