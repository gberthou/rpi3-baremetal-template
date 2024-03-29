.section .text
.global start
start:
    ;@ Set interrupt vector base address to 0xFFFF0000
    ;@mrc p15, 0, r0, c1, c0, 0
    ;@orr r0, #0x2000 ;@ r0 |= (1 << 13) V bit
    ;@mcr p15, 0, r0, c1, c0, 0
    
    ;@ Disable IRQ
    ;@mov r0, #0xC0
    ;@msr cpsr_c, r0

    ;@ Init IRQ stack
    cps #0x12
    ldr sp, irqStack

    ;@ Init Abort stack
    cps #0x17
    ldr sp, abortStack

    ;@ Init User stack
    cps #0x1f
    ldr sp, usrStack

    ;@ Init SVC stack
    cps #0x13    
    ldr sp, svcStack

    bl main ;@ Squadala!

done:
    b done

.global __stack_begin
__stack_begin:
abortStack: .word 0x00006700
irqStack:   .word 0x00006800
svcStack:   .word 0x00007000
usrStack:   .word 0x00007800
