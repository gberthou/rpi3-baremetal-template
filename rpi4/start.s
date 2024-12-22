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

    mov x4, #0xe0

    adr x5, start_core1
    // Comment out to "disable" core 1
    str w5, [x4]

    adr x5, start_core2
    // Comment out to "disable" core 2
    str w5, [x4, #0x8]

    adr x5, start_core3
    // Comment out to "disable" core 3
    str w5, [x4, #0x10]

    // This portion of code is only run by core 0
    sev // Signal event to the other cores to ensure they are awake
    clrex

    // Branch to main0 into EL1 instead of EL2 using ERET
    bl mmu_init_registers
    adr x0, core0stack
    adr x1, main0

// x0: stack pointer
// x1: main address
branch_to_main:
    msr SP_EL1, x0
    msr ELR_EL2, x1

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

start_core1:
    adr x0, core1stack
    mov sp, x0
    bl mmu_init_registers
    adr x0, core1stack
    adr x1, main1
    b branch_to_main

start_core2:
    adr x0, core2stack
    mov sp, x0
    bl mmu_init_registers
    adr x0, core2stack
    adr x1, main2
    b branch_to_main

start_core3:
    adr x0, core3stack
    mov sp, x0
    bl mmu_init_registers
    adr x0, core3stack
    adr x1, main3
    b branch_to_main

.section .stack
.org 0x800
core0stack: .space 0x800
core1stack: .space 0x800
core2stack: .space 0x800
core3stack: .space 0x800
