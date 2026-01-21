#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

// System constants
#define KERNEL_VERSION "1.0.0"
#define PAGE_SIZE 4096
#define MAX_PROCESSES 1024
#define MAX_FILES 4096
#define MAX_DRIVERS 256

// Process types
#define PROCESS_KERNEL 0
#define PROCESS_SYSTEM 1
#define PROCESS_USER 2

// Memory management
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} memory_map_entry_t;

// Process control block
typedef struct process {
    uint32_t pid;
    uint32_t ppid;
    uint32_t state;
    uint32_t priority;
    uint64_t rsp;
    uint64_t cr3;
    char name[256];
    struct process* next;
} process_t;

// File descriptor
typedef struct {
    uint32_t fd;
    uint32_t flags;
    uint64_t offset;
    void* inode;
} file_descriptor_t;

// Driver interface
typedef struct driver {
    char name[64];
    uint32_t type;
    int (*init)(void);
    int (*read)(void* buffer, size_t size);
    int (*write)(const void* buffer, size_t size);
    int (*ioctl)(uint32_t cmd, void* arg);
    struct driver* next;
} driver_t;

// System call numbers
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_FORK 4
#define SYS_EXEC 5
#define SYS_EXIT 6
#define SYS_WAIT 7
#define SYS_KILL 8
#define SYS_GETPID 9
// ... 190 more system calls

// Function prototypes
void kernel_main(void);
void kernel_panic(const char* message);
void kprintf(const char* format, ...);
void start_system_processes(void);

// Assembly functions
extern void enable_interrupts(void);
extern void disable_interrupts(void);
extern void halt(void);
extern uint64_t read_cr3(void);
extern void write_cr3(uint64_t cr3);

// Subsystem initialization
void memory_init(void);
void interrupt_init(void);
void process_init(void);
void driver_init(void);
void fs_init(void);
void gui_init(void);
void network_init(void);
void security_init(void);

// Core functions
void scheduler_start(void);
void console_write(const char* str);
void gui_emergency_mode(void);
int vsprintf(char* buffer, const char* format, va_list args);

#endif