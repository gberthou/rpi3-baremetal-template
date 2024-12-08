.section .text
.global ResetHandler
ResetHandler:
    ;@ Set interrupt vector base address to 0xFFFF0000
    ;@mrc p15, 0, r0, c1, c0, 0
    ;@orr r0, #0x2000 ;@ r0 |= (1 << 13) V bit
    ;@mcr p15, 0, r0, c1, c0, 0

    ;@ Disable IRQ
    ;@mov r0, #0xC0
    ;@msr cpsr_c, r0

    ;@ Init IRQ stack
    cps #0x12
    ldr sp, =irqstack

    ;@ Init SVC stack
    cps #0x13
    ldr sp, =svcstack

    ;@ Init Abort stack
    cps #0x17
    ldr sp, =abortstack

    ;@ Init User stack
    cps #0x1f
    ldr sp, =usrstack

    bl main ;@ Squadala!

done:
    b done

.section .stack
.align 16
.space 0x800-4
irqstack: .space 4
.space 0x800-4
svcstack: .space 4
.space 0x800-4
abortstack: .space 4
.space 0x800-4
usrstack: .space 4
