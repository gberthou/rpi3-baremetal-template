.macro basic_exception_handler, el
    .align 7
    .set beg, .
    adr x19, . // X19 = pc
    mrs x20, ESR_EL\el
    mrs x21, FAR_EL\el
    lsr x19, x19, #7
    and x19, x19, #0xf // X19: VBAR_ELx index
    bl huart_init
    mov x0, #\el
    mov x1, x19
    mov x2, x20
    mov x3, x21
    bl huart_print_exception
1:
    wfe
    b 1b
    .if . > beg + 0x80
    .error "Exception handler must be shorter than 0x80"
    .endif
.endm

.macro unlock_cores_legacy, n_core_max
.macro load_core_address, number
    adr x5, start_core\number
.endm
    mov x4, #0xe0

    .altmacro
    .set i, 1
    .rept \n_core_max
        load_core_address %i
        str w5, [x4, #(8 * (i - 1))]
        .set i, i+1
    .endr
    dsb sy
.endm

.macro unlock_cores, n_core_max
    /* https://github.com/raspberrypi/arm-trusted-firmware/blob/332b62e0443b004287d2cfc032a70bba6fb86f35/plat/rpi/common/aarch64/plat_helpers.S
     * https://github.com/raspberrypi/arm-trusted-firmware/blob/332b62e0443b004287d2cfc032a70bba6fb86f35/plat/rpi/rpi5/include/platform_def.h
    */
    mov x0, #0x100 // PLAT_RPI3_TM_ENTRYPOINT
    mov x1, #1
    adr x2, start_secondary_core
    str x2, [x0], #16
    dsb sy

    mov x2, #1 // x2 = next core to wake up
1:
    str x1, [x0], #8 // PLAT_RPI3_TM_HOLD_BASE[x]
    add x2, x2, #1
    cmp x2, #\n_core_max
    ble 1b
    dsb sy
    sev
.endm

.macro restart_from_el3_to_el2
    mrs x0, CurrentEL
    and x0, x0, #0xc
    cmp x0, #0xc
    bne 1f
    mrs x0, SCR_EL3
    orr x0, x0, #(1 << 18) // EEL2
    orr x0, x0, #(1 << 10) // RW
    orr x0, x0, #(1 << 0) // NS
    msr SCR_EL3, x0
    mov x0, #0x9 // EL2
    msr SPSR_EL3, x0
    adr x0, start_secondary_core
    msr ELR_EL3, x0
    isb
    eret
1:
.endm

.section .text.armvector
.align 5
// VBAR_EL2
vbar_el2_ini:
.rept 16
basic_exception_handler 2
.endr
// VBAR_EL1
.align 5
vbar_el1_ini:
.rept 16
basic_exception_handler 1
.endr

.section .text
.global ResetHandler
ResetHandler:
    // Zero bss section
    ldr x0, =__bss_start
    ldr x1, =__bss_end
    mov x2, #0
bssloop:
    cmp x0, x1
    beq bssloopout
    str x2, [x0], #8
    b bssloop
bssloopout:

    // Copy init values for data
    ldr x0, =__data_start
    ldr x1, =__data_end
    ldr x2, =__data_load_start
dataloop:
    cmp x0, x1
    beq launch
    ldr x3, [x2], #8
    str x3, [x0], #8
    b dataloop

launch:
    // Before calling C functions, setup floating-point accesses.
    mov x0, #(3 << 20) // CPTR_EL2.FPEN = 3
    msr CPTR_EL2, x0 // The compiler tends to generate SIMD instructions for
                     // copies (strcpy, etc.)
    msr CPACR_EL1, x0
    isb

    // First, run clock_max_out_arm on a valid stack
    adr x0, core0stack
    mov sp, x0 // set sp of core 0

    bl clock_max_out_arm
    bl mmu_init_descriptors

    clrex

    // Unlock cores 1, 2, 3
.if RPI < 5
    unlock_cores_legacy 3
.else
    unlock_cores 3
.endif

    // Branch to main0 into EL1 instead of EL2 using ERET
    adr x0, core0stack
    adr x1, main0

// x0: stack pointer
// x1: main address
branch_to_main:
    mov x19, x0
    mov x20, x1

    mov sp, x0
    bl mmu_init_registers

    msr SP_EL1, x19
    msr ELR_EL2, x20

    // Prepare VBAR_EL2 and VBAR_EL1
    ldr x0, =vbar_el2_ini
    msr VBAR_EL2, x0
    ldr x0, =vbar_el1_ini
    msr VBAR_EL1, x0

    // Prepare EL1 to run the main(s) instead of EL2
    mov x0, #0x3c5
    msr SPSR_EL2, x0
    mov x0, #((1 << 0) | (1 << 2)) // SCTLR_EL1.{M,C}
    msr SCTLR_EL1, x0
    mrs x0, HCR_EL2
    orr x0, x0, #(1 << 31) // HCR_EL2.RW=1
    msr HCR_EL2, x0
    mov x0, #(3 << 20) // CPACR_EL1.FPEN=3
    msr CPACR_EL1, x0 // The compiler tends to generate SIMD instructions for
                      // copies (strcpy, etc.)
    eret

.if RPI >= 5
start_secondary_core:
    restart_from_el3_to_el2
    mrs x0, MPIDR_EL1
    lsr x0, x0, #8
    and x0, x0, #0xff // x0 = MPIDR_EL1.Aff1 (core number)
    cmp x0, #0
    beq unknown_core // Core 0 is supposed to be the primary core, so it's not
                     // Expected to be 0 here
    cmp x0, #4 // Only cores 0-3 are expected here
    bge unknown_core

    adr x1, jump_vector
    sub x0, x0, #1
    lsl x0, x0, #2
    add x1, x1, x0
    br x1
jump_vector:
    b start_core1
    b start_core2
    b start_core3
unknown_core:
    wfe
    b unknown_core
.endif

start_core1:
    adr x0, core1stack
    adr x1, main1
    b branch_to_main

start_core2:
    adr x0, core2stack
    adr x1, main2
    b branch_to_main

start_core3:
    adr x0, core3stack
    adr x1, main3
    b branch_to_main

.section .stack
.org 0x800
core0stack: .space 0x800
core1stack: .space 0x800
core2stack: .space 0x800
core3stack: .space 0x800
