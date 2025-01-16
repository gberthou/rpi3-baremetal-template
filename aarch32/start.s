.macro _isb
.if RPI == 1
    mcr p15, 0, r0, c7, c5, 4
.else
    isb
.endif
.endm

.section .text.armvector
.align 5
.if RPI > 1
vbar_ini_hyp:
.rept 8
bl basic_exception_handler_hyp
.endr
.endif
vbar_ini_pl1:
.rept 8
bl basic_exception_handler_pl1
.endr
.if RPI > 1
basic_exception_handler_hyp:
    mrc p15, 4, r5, c5, c2, 0 // HSR
    mrc p15, 4, r6, c6, c0, 0 // HDFAR
.endif
basic_exception_handler_epilogue:
    sub r4, lr, #4 // R4 = PC of the VBAR entry
    lsr r4, r4, #2
    and r4, r4, #0x7 // R4: VBAR index
    bl init_is_qemu
    bl huart_init
    mrs r0, cpsr
    and r0, r0, #0xf
    mov r1, r4
    mov r2, r5
    mov r3, r6
    bl huart_print_exception
1:
    wfe
    b 1b
basic_exception_handler_pl1:
    mrc p15, 0, r5, c5, c0, 0 // DFSR
    mrc p15, 0, r6, c6, c0, 0 // DFAR
    b basic_exception_handler_epilogue

.section .text
.if RPI > 1
is_hyp:
    mrs r0, cpsr
    and r0, r0, #0xf
    cmp r0, #0xa
    moveq r0, #1
    movne r0, #0
    bx lr
.endif

.global ResetHandler
ResetHandler:
    // Zero bss section
    ldr r0, =__bss_start
    ldr r1, =__bss_end
    mov r2, #0
bssloop:
    cmp r0, r1
    beq bssloopout
    str r2, [r0], #4
    b bssloop
bssloopout:

    // Copy rodata into data
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
    // Before calling C functions, setup floating-point accesses.
    mov r0, #(3 << 20) // FPEN
    mcr p15, 0, r0, c1, c0, 2 // CPACR
    _isb

    // First, run clock_max_out_arm on a valid stack
    ldr sp, =core0stack     // Set sp of core 0 (SVC for qemu, HYP for real
                            // hardware)
    bl init_is_qemu
    bl clock_max_out_arm
    bl mmu_init_descriptors

.if RPI > 1
// Base address shown at:
// https://github.com/raspberrypi/tools/blob/master/armstubs/armstub7.S
.if RPI >= 4
    ldr r0, =is_qemu
    ldr r0,[r0]
    cmp r0, #1
    moveq r4, #0x40000000
    movne r4,     #0xff000000
    addne r4, r4, #0x00800000
.else
    mov r4, #0x40000000
.endif
    ldr r5, =start_core1
    // Comment out to "disable" core 1
    str r5, [r4, #0x9c]

    ldr r5, =start_core2
    // Comment out to "disable" core 2
    str r5, [r4, #0xac]

    ldr r5, =start_core3
    // Comment out to "disable" core 3
    str r5, [r4, #0xbc]
    dsb sy
    sev // Signal event to the other cores to ensure they are awake
.endif
    // This portion of code is only run by core 0
    clrex

    ldr r0, =core0stack
    ldr r1, =main0

// r0: stack pointer
// r1: main address
branch_to_main:
    mov r4, r0
    mov r5, r1

    mov sp, r0
    bl mmu_init_registers

    ldr r1, =irqstack // Set IRQ stack
.if RPI == 1
    cps #0x12 // IRQ
    mov sp, r1
    cps #0x1f
.else
    msr sp_irq, r1
.endif

    // Prepare HVBAR if in HYP mode
.if RPI > 1
    bl is_hyp
    cmp r0, #0
    beq prepare_vbar_pl1
    ldr r0, =vbar_ini_hyp
    mcr 15, 4, r0, c6, c0, 0 // HVBAR
.endif
prepare_vbar_pl1:
    // Prepare VBAR
    ldr r0, =vbar_ini_pl1
    mcr 15, 0, r0, c12, c0, 0 // VBAR

    // Prepare SVC mode
    mov r0, #((1 << 5) | (1 << 0) | (1 << 2)) // CP15BEN, M, C
    orr r0, #(1 << 12) // I
    mcr 15, 0, r0, c1, c0, 0 // SCTLR
    mov r0, #(3 << 20) // FPEN
    mcr p15, 0, r0, c1, c0, 2 // CPACR
.if RPI > 1
    bl is_hyp
    cmp r0, #0
    beq jump_to_main_pl1
    msr sp_svc, r4
    msr elr_hyp, r5
    mov r0, #0x1d3 // M=AArch32 SVC; AIF=0b111
    msr spsr_hyp, r0
    _isb
    eret
.endif
jump_to_main_pl1:
    mov sp, r4
    _isb
    blx r5

.if RPI > 1
start_core1:
    ldr r0, =core1stack
    ldr r1, =main1
    b branch_to_main

start_core2:
    ldr r0, =core2stack
    ldr r1, =main2
    b branch_to_main

start_core3:
    ldr r0, =core3stack
    ldr r1, =main3
    b branch_to_main
.endif

.section .stack
.org 0x1000
irqstack: .space 0x800
core0stack: .space 0x800
.if RPI > 1
core1stack: .space 0x800
core2stack: .space 0x800
core3stack: .space 0x800
.endif
