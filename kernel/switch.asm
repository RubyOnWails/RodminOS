[bits 64]
global context_switch

; context_switch(context **old, context *new)
; rdi: address of pointer to old context
; rsi: pointer to new context
context_switch:
    ; Save old callee-saved registers
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save old stack pointer
    mov [rdi], rsp

    ; Load new stack pointer
    mov rsp, rsi

    ; Restore new callee-saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    ret
