.arm
.global init_vectors
.global reset_handler
.global undef_handler
.global svc_handler
.global prefetch_abort_handler
.global data_abort_handler
.global irq_handler
.global fiq_handler
.global uart_driver_init
.global c_entry
.global invalid_excp
.global handle_irq

.section .vectors, "ax"
init_vectors:
    ldr pc, =reset_handler
    ldr pc, =undef_handler
    ldr pc, =svc_handler
    ldr pc, =prefetch_abort_handler
    ldr pc, =data_abort_handler
    ldr pc, =0
    ldr pc, =irq_handler
    ldr pc, =fiq_handler

reset_handler:
    ldr sp, =_initial_sp
    msr cpsr_c, #0xD2
    ldr sp, =_initial_irq_sp
    msr cpsr_c, #0xDF
    ldr sp, =_initial_sp
    bl uart_driver_init
    bl c_entry
1:
    wfi
    b 1b

undef_handler:
    stmfd sp!, {r0-r3, r12, lr}
    bl invalid_excp
    ldmfd sp!, {r0-r3, r12, lr}
    subs pc, lr, #4

svc_handler:
    stmfd sp!, {r0-r3, r12, lr}
    bl invalid_excp
    ldmfd sp!, {r0-r3, r12, lr}
    subs pc, lr, #4

prefetch_abort_handler:
    stmfd sp!, {r0-r3, r12, lr}
    bl invalid_excp
    ldmfd sp!, {r0-r3, r12, lr}
    subs pc, lr, #4

data_abort_handler:
    stmfd sp!, {r0-r3, r12, lr}
    bl invalid_excp
    ldmfd sp!, {r0-r3, r12, lr}
    subs pc, lr, #4

irq_handler:
    stmfd sp!, {r0-r3, r12, lr}
    bl handle_irq
    ldmfd sp!, {r0-r3, r12, lr}
    subs pc, lr, #4

fiq_handler:
    stmfd sp!, {r0-r3, r12, lr}
    bl invalid_excp
    ldmfd sp!, {r0-r3, r12, lr}
    subs pc, lr, #4
