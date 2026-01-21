#include "proc.h"
#include "kernel.h"

extern process_t* current_proc;
extern process_t processes[MAX_PROCESSES];
extern void context_switch(context_t** old, context_t* new);

void scheduler(void) {
    while(1) {
        // Enable interrupts to allow timer preemption
        __asm__ __volatile__ ("sti");
        
        process_t* p = NULL;
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (processes[i].state == PROC_STATE_READY) {
                p = &processes[i];
                break;
            }
        }
        
        if (p) {
            p->state = PROC_STATE_RUNNING;
            current_proc = p;
            
            // Switch address space
            // vmm_switch_page_table(p->page_table);
            
            // We need a placeholder for the kernel's own context when no process is running
            static context_t* kernel_context;
            context_switch(&kernel_context, p->context);
            
            // When we return here, the process has finished or yielded
            current_proc = NULL;
        } else {
            // Idle loop
            __asm__ __volatile__ ("hlt");
        }
    }
}

void scheduler_yield(void) {
    if (current_proc) {
        current_proc->state = PROC_STATE_READY;
        // Trigger context switch back to scheduler
        // This usually involves saving the current state and jumping to scheduler loop
    }
}
