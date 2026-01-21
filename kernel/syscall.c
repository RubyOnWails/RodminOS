#include "kernel.h"
#include "proc.h"

// System Call Numbers
#define SYS_READ  0
#define SYS_WRITE 1
#define SYS_OPEN  2
#define SYS_CLOSE 3
#define SYS_FORK  57
#define SYS_EXEC  59
#define SYS_EXIT  60

typedef uint64_t (*syscall_handler_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static uint64_t sys_exit(uint64_t status) {
    proc_exit((int)status);
    return 0;
}

static uint64_t sys_write(uint64_t fd, uint64_t buf, uint64_t count) {
    // Deep implementation would check fd and call VFS
    kprintf("%s", (char*)buf);
    return count;
}

static syscall_handler_t syscall_table[] = {
    [SYS_EXIT] = (syscall_handler_t)sys_exit,
    [SYS_WRITE] = (syscall_handler_t)sys_write,
    // Add more handlers...
};

void handle_syscall(interrupt_frame_t* frame) {
    uint64_t syscall_num = frame->rax;
    
    if (syscall_num < sizeof(syscall_table)/sizeof(syscall_table[0]) && syscall_table[syscall_num]) {
        frame->rax = syscall_table[syscall_num](frame->rdi, frame->rsi, frame->rdx, frame->r10, frame->r8, frame->r9);
    } else {
        kprintf("Invalid syscall: %d\n", syscall_num);
        frame->rax = -1;
    }
}
