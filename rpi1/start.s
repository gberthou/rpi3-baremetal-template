.section .text
.global ResetHandler
ResetHandler:
    ;@ Zero bss section
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
bssloop:
    cmp r0, r1
    beq bssloopout
    str r2, [r0], #4
    b bssloop
bssloopout:

    ;@ Copy rodata into data
    ldr r0, =__data_start
    ldr r1, =__data_end
    ldr r2, =__data_load_start
dataloop:
    cmp r0, r1
    beq launch
    ldr r3, [r2], #4
    str r3, [r0], #4
    b dataloop

launch:
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
.org 0x1000
irqstack: .space 0x800
svcstack: .space 0x800
abortstack: .space 0x800
usrstack: .space 0x800
