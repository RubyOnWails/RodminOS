#include "process.h"
#include "memory.h"
#include "kernel.h"

static process_t* process_list = NULL;
static process_t* current_process = NULL;
static uint32_t next_pid = 1;
static uint32_t process_count = 0;

// Scheduler queues
static process_queue_t ready_queues[MAX_PRIORITY_LEVELS];
static process_queue_t blocked_queue;
static process_queue_t zombie_queue;

// Process table
static process_t process_table[MAX_PROCESSES];
static bool process_slots[MAX_PROCESSES];

void process_init(void) {
    // Initialize process table
    for(int i = 0; i < MAX_PROCESSES; i++) {
        process_slots[i] = false;
    }
    
    // Initialize scheduler queues
    for(int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        ready_queues[i].head = NULL;
        ready_queues[i].tail = NULL;
        ready_queues[i].count = 0;
    }
    
    blocked_queue.head = NULL;
    blocked_queue.tail = NULL;
    blocked_queue.count = 0;
    
    zombie_queue.head = NULL;
    zombie_queue.tail = NULL;
    zombie_queue.count = 0;
    
    // Create kernel idle process
    create_idle_process();
    
    kprintf("Process management initialized\n");
}

uint32_t process_create(const char* path, uint32_t priority, uint32_t type) {
    // Find free process slot
    int slot = -1;
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(!process_slots[i]) {
            slot = i;
            process_slots[i] = true;
            break;
        }
    }
    
    if(slot == -1) return 0; // No free slots
    
    process_t* proc = &process_table[slot];
    
    // Initialize process structure
    proc->pid = next_pid++;
    proc->ppid = current_process ? current_process->pid : 0;
    proc->state = PROCESS_READY;
    proc->priority = priority;
    proc->type = type;
    proc->time_slice = DEFAULT_TIME_SLICE;
    proc->cpu_time = 0;
    proc->start_time = get_system_time();
    
    strncpy(proc->name, path, 255);
    proc->name[255] = '\0';
    
    // Create address space
    proc->page_table = create_page_table();
    if(!proc->page_table) {
        process_slots[slot] = false;
        return 0;
    }
    
    // Allocate stack
    proc->stack_base = alloc_user_stack();
    proc->stack_size = USER_STACK_SIZE;
    
    // Load executable
    if(!load_executable(proc, path)) {
        destroy_page_table(proc->page_table);
        free_user_stack(proc->stack_base);
        process_slots[slot] = false;
        return 0;
    }
    
    // Initialize registers
    proc->registers.rsp = proc->stack_base + proc->stack_size - 8;
    proc->registers.rip = proc->entry_point;
    proc->registers.rflags = 0x202; // Interrupts enabled
    
    // Initialize file descriptors
    for(int i = 0; i < MAX_FDS_PER_PROCESS; i++) {
        proc->fds[i].fd = -1;
        proc->fds[i].flags = 0;
        proc->fds[i].offset = 0;
        proc->fds[i].inode = NULL;
    }
    
    // Add to ready queue
    add_to_ready_queue(proc);
    
    process_count++;
    
    kprintf("Created process %d: %s\n", proc->pid, proc->name);
    return proc->pid;
}

void scheduler_start(void) {
    // Enable timer interrupt for preemptive scheduling
    setup_scheduler_timer();
    
    // Start scheduling
    schedule();
}

void schedule(void) {
    process_t* next = select_next_process();
    
    if(next && next != current_process) {
        process_t* prev = current_process;
        current_process = next;
        
        // Update process states
        if(prev && prev->state == PROCESS_RUNNING) {
            prev->state = PROCESS_READY;
            add_to_ready_queue(prev);
        }
        
        next->state = PROCESS_RUNNING;
        next->time_slice = calculate_time_slice(next);
        
        // Context switch
        context_switch(prev, next);
    }
}

process_t* select_next_process(void) {
    // Round-robin within priority levels
    for(int priority = 0; priority < MAX_PRIORITY_LEVELS; priority++) {
        if(ready_queues[priority].count > 0) {
            process_t* proc = dequeue_process(&ready_queues[priority]);
            return proc;
        }
    }
    
    // Return idle process if no other process is ready
    return get_idle_process();
}

void context_switch(process_t* from, process_t* to) {
    // Save current process state
    if(from) {
        save_process_state(from);
    }
    
    // Load new process state
    load_process_state(to);
    
    // Switch address space
    write_cr3((uint64_t)to->page_table);
    
    // Update TSS for kernel stack
    update_tss(to);
}

void save_process_state(process_t* proc) {
    // Save registers (called from assembly interrupt handler)
    // This is a simplified version - actual implementation would
    // save all registers from interrupt stack frame
    proc->registers.rsp = get_current_rsp();
    proc->registers.rip = get_current_rip();
    proc->registers.rflags = get_current_rflags();
}

void load_process_state(process_t* proc) {
    // Load registers (will be restored by interrupt return)
    set_current_rsp(proc->registers.rsp);
    set_current_rip(proc->registers.rip);
    set_current_rflags(proc->registers.rflags);
}

void process_exit(uint32_t exit_code) {
    if(!current_process) return;
    
    current_process->state = PROCESS_ZOMBIE;
    current_process->exit_code = exit_code;
    current_process->exit_time = get_system_time();
    
    // Close all file descriptors
    for(int i = 0; i < MAX_FDS_PER_PROCESS; i++) {
        if(current_process->fds[i].fd != -1) {
            close_fd(&current_process->fds[i]);
        }
    }
    
    // Free memory (except page table for parent to read exit code)
    free_process_memory(current_process);
    
    // Wake up parent if waiting
    wake_waiting_parent(current_process);
    
    // Add to zombie queue
    enqueue_process(&zombie_queue, current_process);
    
    // Schedule next process
    schedule();
}

uint32_t process_fork(void) {
    if(!current_process) return 0;
    
    // Create child process
    uint32_t child_pid = process_create(current_process->name, 
                                       current_process->priority,
                                       current_process->type);
    
    if(child_pid == 0) return 0;
    
    process_t* child = get_process_by_pid(child_pid);
    if(!child) return 0;
    
    // Copy parent's address space
    copy_address_space(current_process, child);
    
    // Copy file descriptors
    for(int i = 0; i < MAX_FDS_PER_PROCESS; i++) {
        if(current_process->fds[i].fd != -1) {
            child->fds[i] = current_process->fds[i];
            // Increment reference count for shared files
            increment_file_ref(child->fds[i].inode);
        }
    }
    
    // Set return values
    // Parent gets child PID, child gets 0
    child->registers.rax = 0;
    
    return child_pid;
}

int process_exec(const char* path, char* const argv[], char* const envp[]) {
    if(!current_process) return -1;
    
    // Save process info
    uint32_t pid = current_process->pid;
    uint32_t ppid = current_process->ppid;
    
    // Clear address space
    clear_address_space(current_process);
    
    // Load new executable
    if(!load_executable(current_process, path)) {
        process_exit(-1);
        return -1;
    }
    
    // Setup arguments and environment
    setup_process_args(current_process, argv, envp);
    
    // Reset registers
    current_process->registers.rsp = current_process->stack_base + current_process->stack_size - 8;
    current_process->registers.rip = current_process->entry_point;
    current_process->registers.rflags = 0x202;
    
    strncpy(current_process->name, path, 255);
    
    return 0;
}

uint32_t process_wait(uint32_t pid, int* status) {
    if(!current_process) return 0;
    
    // Find child process
    process_t* child = NULL;
    if(pid == 0) {
        // Wait for any child
        child = find_any_child(current_process->pid);
    } else {
        child = get_process_by_pid(pid);
        if(!child || child->ppid != current_process->pid) {
            return 0; // Not a child
        }
    }
    
    if(!child) return 0;
    
    // If child is zombie, collect it
    if(child->state == PROCESS_ZOMBIE) {
        if(status) *status = child->exit_code;
        uint32_t child_pid = child->pid;
        cleanup_zombie_process(child);
        return child_pid;
    }
    
    // Block until child exits
    current_process->state = PROCESS_BLOCKED;
    current_process->wait_pid = pid;
    add_to_blocked_queue(current_process);
    
    schedule();
    
    // When we return here, child has exited
    if(status) *status = current_process->wait_status;
    return current_process->wait_result;
}

void process_kill(uint32_t pid, int signal) {
    process_t* proc = get_process_by_pid(pid);
    if(!proc) return;
    
    // Handle different signals
    switch(signal) {
        case SIGTERM:
        case SIGKILL:
            proc->state = PROCESS_ZOMBIE;
            proc->exit_code = -signal;
            proc->exit_time = get_system_time();
            
            // Remove from ready queue if present
            remove_from_ready_queue(proc);
            
            // Add to zombie queue
            enqueue_process(&zombie_queue, proc);
            
            // Wake up parent
            wake_waiting_parent(proc);
            break;
            
        case SIGSTOP:
            if(proc->state == PROCESS_RUNNING || proc->state == PROCESS_READY) {
                proc->state = PROCESS_BLOCKED;
                remove_from_ready_queue(proc);
                add_to_blocked_queue(proc);
            }
            break;
            
        case SIGCONT:
            if(proc->state == PROCESS_BLOCKED) {
                proc->state = PROCESS_READY;
                remove_from_blocked_queue(proc);
                add_to_ready_queue(proc);
            }
            break;
    }
}

// Queue management functions
void enqueue_process(process_queue_t* queue, process_t* proc) {
    proc->next = NULL;
    
    if(queue->tail) {
        queue->tail->next = proc;
    } else {
        queue->head = proc;
    }
    
    queue->tail = proc;
    queue->count++;
}

process_t* dequeue_process(process_queue_t* queue) {
    if(!queue->head) return NULL;
    
    process_t* proc = queue->head;
    queue->head = proc->next;
    
    if(!queue->head) {
        queue->tail = NULL;
    }
    
    queue->count--;
    proc->next = NULL;
    
    return proc;
}

void add_to_ready_queue(process_t* proc) {
    if(proc->priority >= MAX_PRIORITY_LEVELS) {
        proc->priority = MAX_PRIORITY_LEVELS - 1;
    }
    
    enqueue_process(&ready_queues[proc->priority], proc);
}

void remove_from_ready_queue(process_t* proc) {
    process_queue_t* queue = &ready_queues[proc->priority];
    
    if(queue->head == proc) {
        dequeue_process(queue);
        return;
    }
    
    process_t* current = queue->head;
    while(current && current->next != proc) {
        current = current->next;
    }
    
    if(current) {
        current->next = proc->next;
        if(queue->tail == proc) {
            queue->tail = current;
        }
        queue->count--;
        proc->next = NULL;
    }
}

// Utility functions
process_t* get_process_by_pid(uint32_t pid) {
    for(int i = 0; i < MAX_PROCESSES; i++) {
        if(process_slots[i] && process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

uint32_t get_current_pid(void) {
    return current_process ? current_process->pid : 0;
}

void get_process_list(process_info_t* list, uint32_t* count) {
    uint32_t index = 0;
    
    for(int i = 0; i < MAX_PROCESSES && index < *count; i++) {
        if(process_slots[i]) {
            process_t* proc = &process_table[i];
            
            list[index].pid = proc->pid;
            list[index].ppid = proc->ppid;
            list[index].state = proc->state;
            list[index].priority = proc->priority;
            list[index].cpu_time = proc->cpu_time;
            strncpy(list[index].name, proc->name, 255);
            
            index++;
        }
    }
    
    *count = index;
}