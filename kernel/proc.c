#include "proc.h"
#include "memory.h"
#include "kernel.h"
#include <string.h>

static process_t processes[MAX_PROCESSES];
static uint32_t next_pid = 1;
process_t* current_proc = NULL;

void proc_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        processes[i].state = PROC_STATE_UNUSED;
    }
}

static process_t* alloc_proc(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i].state == PROC_STATE_UNUSED) {
            process_t* p = &processes[i];
            p->pid = next_pid++;
            p->state = PROC_STATE_EMBRYO;
            
            // Allocate kernel stack
            p->kstack = (uint64_t)kmalloc(KERNEL_STACK_SIZE);
            memset((void*)p->kstack, 0, KERNEL_STACK_SIZE);
            
            // Prepare context at top of stack for return from context_switch
            uint64_t sp = p->kstack + KERNEL_STACK_SIZE;
            sp -= sizeof(context_t);
            p->context = (context_t*)sp;
            memset(p->context, 0, sizeof(context_t));
            
            return p;
        }
    }
    return NULL;
}

process_t* proc_create(const char* name) {
    process_t* p = alloc_proc();
    if (!p) return NULL;
    
    strncpy(p->name, name, 255);
    p->state = PROC_STATE_READY;
    
    // Allocate page table (deep implementation would use vmm_create_address_space)
    // p->page_table = vmm_create_address_space();
    
    return p;
}

void proc_exit(int status) {
    if (current_proc) {
        current_proc->state = PROC_STATE_ZOMBIE;
        current_proc->exit_status = status;
        // Notify parent, swap context to scheduler
        // scheduler_yield();
    }
}
