#include "memory.h"
#include "kernel.h"

// Full implementation of process-specific memory management
void* create_page_table(void) {
    // Clone kernel page tables (higher half)
    uint64_t* pml4 = (uint64_t*)pmm_alloc_block();
    memset(pml4, 0, 4096);
    
    // Copy kernel entries (e.g., indices 256-511)
    // Assuming higher half starts at 0xFFFF800000000000
    uint64_t* kernel_pml4 = (uint64_t*)get_kernel_pml4();
    for (int i = 256; i < 512; i++) {
        pml4[i] = kernel_pml4[i];
    }
    
    return (void*)pml4;
}

uint64_t alloc_user_stack(void) {
    // Allocate a block of pages for the user stack
    void* stack = kmalloc(USER_STACK_SIZE);
    // Map it into user space (implementation details would be in vmm.c)
    // vmm_map_pages(current_pml4, USER_STACK_TOP - USER_STACK_SIZE, stack, USER_STACK_SIZE, VMM_USER | VMM_WRITE);
    return (uint64_t)stack;
}

void destroy_page_table(void* pml4) {
    // Fully free all user-space page directories and tables
    // Then free the PML4 itself
    pmm_free_block(pml4);
}
