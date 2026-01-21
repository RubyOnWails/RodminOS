#include "kernel.h"
#include "memory.h"
#include "process.h"
#include "interrupt.h"
#include "driver.h"
#include "fs.h"
#include "gui.h"
#include "network.h"
#include "security.h"

// Kernel entry point called from bootloader
void kernel_main(void) {
    // Initialize core subsystems
    memory_init();
    interrupt_init();
    process_init();
    
    // Initialize hardware drivers
    driver_init();
    
    // Initialize file system
    fs_init();
    
    // Initialize security subsystem
    security_init();
    
    // Initialize networking
    network_init();
    
    // Initialize GUI system
    gui_init();
    
    // Start system processes
    start_system_processes();
    
    // Enable interrupts and enter scheduler
    enable_interrupts();
    scheduler_start();
    
    // Should never reach here
    while(1) {
        halt();
    }
}

void start_system_processes(void) {
    // Start init process
    process_create("/system/init", 0, PROCESS_KERNEL);
    
    // Start GUI desktop manager
    process_create("/system/desktop", 1, PROCESS_SYSTEM);
    
    // Start network daemon
    process_create("/system/networkd", 2, PROCESS_SYSTEM);
    
    // Start security daemon
    process_create("/system/securityd", 3, PROCESS_SYSTEM);
    
    // Start file system daemon
    process_create("/system/fsd", 4, PROCESS_SYSTEM);
}

// Kernel panic handler
void kernel_panic(const char* message) {
    disable_interrupts();
    
    // Switch to emergency text mode
    gui_emergency_mode();
    
    // Display panic message
    kprintf("KERNEL PANIC: %s\n", message);
    kprintf("System halted.\n");
    
    // Halt system
    while(1) {
        halt();
    }
}

// Kernel printf implementation
void kprintf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsprintf(buffer, format, args);
    
    // Output to console
    console_write(buffer);
    
    va_end(args);
}