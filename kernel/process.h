#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include <stdbool.h>

// Process states
#define PROCESS_READY    0
#define PROCESS_RUNNING  1
#define PROCESS_BLOCKED  2
#define PROCESS_ZOMBIE   3

// Process priorities
#define MAX_PRIORITY_LEVELS 8
#define DEFAULT_PRIORITY 4
#define DEFAULT_TIME_SLICE 10 // milliseconds

// Process limits
#define MAX_FDS_PER_PROCESS 256
#define USER_STACK_SIZE (1024 * 1024) // 1MB

// Signals
#define SIGTERM 15
#define SIGKILL 9
#define SIGSTOP 19
#define SIGCONT 18

// CPU registers structure
typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip, rflags;
    uint64_t cs, ds, es, fs, gs, ss;
} cpu_registers_t;

// Process structure
typedef struct process {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    uint32_t priority;
    uint32_t type;
    
    // CPU state
    cpu_registers_t registers;
    uint32_t time_slice;
    uint64_t cpu_time;
    uint64_t start_time;
    uint64_t exit_time;
    int exit_code;
    
    // Memory management
    void* page_table;
    uint64_t stack_base;
    uint64_t stack_size;
    uint64_t heap_base;
    uint64_t heap_size;
    uint64_t entry_point;
    
    // File descriptors
    file_descriptor_t fds[MAX_FDS_PER_PROCESS];
    
    // Process info
    char name[256];
    char cwd[512];
    
    // Synchronization
    uint32_t wait_pid;
    int wait_status;
    uint32_t wait_result;
    
    // Linked list
    struct process* next;
} process_t;

// Process queue
typedef struct {
    process_t* head;
    process_t* tail;
    uint32_t count;
} process_queue_t;

// Process information for system calls
typedef struct {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    uint32_t priority;
    uint64_t cpu_time;
    char name[256];
} process_info_t;

// Function prototypes
void process_init(void);
uint32_t process_create(const char* path, uint32_t priority, uint32_t type);
void process_exit(uint32_t exit_code);
uint32_t process_fork(void);
int process_exec(const char* path, char* const argv[], char* const envp[]);
uint32_t process_wait(uint32_t pid, int* status);
void process_kill(uint32_t pid, int signal);

// Scheduler functions
void scheduler_start(void);
void schedule(void);
process_t* select_next_process(void);
void context_switch(process_t* from, process_t* to);
void save_process_state(process_t* proc);
void load_process_state(process_t* proc);

// Process management
process_t* get_process_by_pid(uint32_t pid);
uint32_t get_current_pid(void);
process_t* get_current_process(void);
void get_process_list(process_info_t* list, uint32_t* count);

// Queue management
void enqueue_process(process_queue_t* queue, process_t* proc);
process_t* dequeue_process(process_queue_t* queue);
void add_to_ready_queue(process_t* proc);
void remove_from_ready_queue(process_t* proc);
void add_to_blocked_queue(process_t* proc);
void remove_from_blocked_queue(process_t* proc);

// Memory management for processes
uint64_t alloc_user_stack(void);
void free_user_stack(uint64_t stack_base);
bool load_executable(process_t* proc, const char* path);
void copy_address_space(process_t* parent, process_t* child);
void clear_address_space(process_t* proc);
void free_process_memory(process_t* proc);

// Process utilities
void create_idle_process(void);
process_t* get_idle_process(void);
uint32_t calculate_time_slice(process_t* proc);
void setup_process_args(process_t* proc, char* const argv[], char* const envp[]);
void setup_scheduler_timer(void);
void update_tss(process_t* proc);

// Synchronization
void wake_waiting_parent(process_t* child);
process_t* find_any_child(uint32_t parent_pid);
void cleanup_zombie_process(process_t* proc);

// File descriptor management
int alloc_fd(process_t* proc);
void free_fd(process_t* proc, int fd);
file_descriptor_t* get_fd(process_t* proc, int fd);
void close_fd(file_descriptor_t* fd);
void increment_file_ref(void* inode);

// System time
uint64_t get_system_time(void);

// Assembly functions
extern uint64_t get_current_rsp(void);
extern uint64_t get_current_rip(void);
extern uint64_t get_current_rflags(void);
extern void set_current_rsp(uint64_t rsp);
extern void set_current_rip(uint64_t rip);
extern void set_current_rflags(uint64_t rflags);

#endif