[bits 64]
global isr_common_stub
extern handle_interrupt

; This is the core of preemptive multitasking
; It saves ALL registers onto the stack, creating an interrupt_frame_t
isr_common_stub:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rdi, rsp ; Pass handle to registers
    call handle_interrupt

    ; After handle_interrupt returns, we might have switched stacks!
    ; In schedule(), we change current_process->registers.rsp

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 16 ; Skip error code and interrupt number
    iretq
