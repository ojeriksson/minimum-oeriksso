.syntax unified
.cpu cortex-a9
.arm

.global minemu_irq_trampoline
.type minemu_irq_trampoline, %function

.extern minemu_irq_dispatch


minemu_irq_trampoline:

    /*
     * Allocate one struct minemu_trap_frame.
     * sizeof(struct minemu_trap_frame) == 88.
     */
    sub sp, sp, #88


    /*
     * Save r0-r12.
     *
     * trap_frame.r[0] begins at offset 0.
     */
    stmia sp, {r0-r12}


    /*
     * Save IRQ-mode LR.
     *
     * return_lr is at offset 52.
     */
    str lr, [sp, #52]


    /*
     * Save SPSR.
     *
     * spsr is at offset 56.
     */
    mrs r1, spsr
    str r1, [sp, #56]


    /*
     * Read the interrupt controller's claim register.
     *
     * Interrupt controller base: 0x10000000
     * claim offset:              0x08
     *
     * Store the interrupt source ID in exception_id,
     * which is at offset 60.
     */
    ldr r1, =0x10000000
    ldr r1, [r1, #8]
    str r1, [sp, #60]


    /*
     * For an IRQ, LR_irq points 4 bytes beyond the
     * interrupted instruction.
     *
     * Store the interrupted PC in fault_pc at offset 64.
     */
    sub r1, lr, #4
    str r1, [sp, #64]


    /*
     * IRQs do not use DFSR or DFAR, so clear them.
     *
     * dfsr: offset 68
     * dfar: offset 72
     */
    mov r1, #0
    str r1, [sp, #68]
    str r1, [sp, #72]


    /*
     * Save the interrupted mode's banked SP and LR.
     *
     * user_sp: offset 76
     * user_lr: offset 80
     *
     * The ^ causes the user/system-mode banked registers
     * to be accessed instead of IRQ-mode SP/LR.
     */
    add r1, sp, #76
    stmia r1, {sp, lr}^


    /*
     * Clear reserved field at offset 84.
     */
    mov r1, #0
    str r1, [sp, #84]


    /*
     * First argument to minemu_irq_dispatch is passed in r0.
     * Pass a pointer to this trap frame.
     */
    mov r0, sp
    bl minemu_irq_dispatch


    /*
     * minemu_irq_dispatch returns the frame to restore in r0.
     *
     * For this homework it will normally be the same frame,
     * but later it could be a different one.
     */
    mov sp, r0


    /*
     * Restore IRQ LR and SPSR.
     */
    ldr lr, [sp, #52]

    ldr r1, [sp, #56]
    msr spsr_cxsf, r1


    /*
     * Restore the interrupted mode's banked SP and LR.
     */
    add r1, sp, #76
    ldmia r1, {sp, lr}^


    /*
     * Restore r0-r12.
     */
    ldmia sp, {r0-r12}


    /*
     * Remove the trap frame from the IRQ stack.
     */
    add sp, sp, #88


    /*
     * Return from IRQ.
     *
     * subs restores CPSR from SPSR because PC is the
     * destination register, and LR_irq requires the -4
     * adjustment for IRQ return.
     */
    subs pc, lr, #4


.size minemu_irq_trampoline, .-minemu_irq_trampoline