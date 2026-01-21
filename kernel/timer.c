#include "kernel.h"
#include "process.h"

// APIC/PIT Timer Setup for Preemptive Scheduling
void setup_scheduler_timer(void) {
    // Using PIT (Programmable Interval Timer) for simplicity but high frequency
    uint32_t divisor = 1193182 / 100; // 100 Hz (10ms)
    outb(0x43, 0x36);             // Command register
    outb(0x40, divisor & 0xFF);   // Low byte
    outb(0x40, (divisor >> 8) & 0xFF); // High byte
    
    kprintf("Scheduler timer initialized (100Hz)\n");
}

// Timer Interrupt Handler (called from ISR)
void timer_handler(interrupt_frame_t* frame) {
    // Increment system time
    // update_system_time();
    
    if (current_process) {
        current_process->cpu_time += 10;
        
        // Check if time slice expired
        if (current_process->time_slice > 0) {
            current_process->time_slice--;
        }
        
        if (current_process->time_slice == 0) {
            // Trigger a schedule
            schedule();
        }
    }
}
