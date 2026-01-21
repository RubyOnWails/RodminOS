#ifndef PROC_H
#define PROC_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_PROCESSES 1024
#define KERNEL_STACK_SIZE 16384
#define USER_STACK_SIZE 65536

typedef enum {
    PROC_STATE_UNUSED = 0,
    PROC_STATE_EMBRYO,
    PROC_STATE_READY,
    PROC_STATE_RUNNING,
    PROC_STATE_BLOCKED,
    PROC_STATE_ZOMBIE
} proc_state_t;

// Saved registers for context switching
typedef struct {
    uint64_t r15, r14, r13, r12;
    uint64_t rbp, rbx;
    uint64_t rip;
} context_t;

// Process Control Block (PCB)
typedef struct process {
    uint32_t pid;
    proc_state_t state;
    uint64_t kstack;
    uint64_t ustack;
    uint64_t page_table;
    context_t* context;
    char name[256];
    
    // Parent/child relationship
    struct process* parent;
    
    // File descriptors
    void* file_table[16];
    
    // Exit status
    int exit_status;
} process_t;

void proc_init(void);
process_t* proc_create(const char* name);
void proc_exit(int status);
int proc_fork(void);
int proc_exec(const char* path, const char** argv);

#endif
