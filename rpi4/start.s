.section .text

/*
enable_svc_mode:
    cpsid if
    // 1. check if core in HYP mode
    mrs r0, cpsr      // r0 = cpsr
    and r1, r0, #0x1f // r1 = mode bits of cpsr (0-4)
    mov r2, #0x1a
    cmp r1, r2        // if mode bits != 0x1a (HYP mode)
    bxne lr           // Return = jump to normal boot without caring about the
                      // mode
    // 2. return from HYP mode
    msr ELR_hyp, lr   // After eret instruction, pc = return address
    bic r1, r0, #0x1f // r1 = cpsr with mode 0x00
    orr r1, r1, #0x13 // r1 = cpsr with mode 0x13 (SVC)
    msr SPSR_hyp, r1  // After eret instruction, cpsr = r1 = old cpsr with mode
                      // SVC
    eret // Return from HYP exception

// Assumes that the caller is in SVC mode (lr register is banked)
init_irq:
    cps #0x12         // Switch to IRQ mode
    ldr sp, =irqstack // Set IRQ stack
    cps #0x13         // Switch to SVC mode

    // Init interrupt vector base address
    mov r0, #0x8000
    mcr p15, 0, r0, c12, c0, 0 // VBAR

    bx lr // Return
*/

.global ResetHandler
ResetHandler:
    ldr x0, =0xfe201000 // UART_BASE
    ldr x1, =0xfe200000 // GPIO_BASE

    // GPFSEL1 = 0x24000 (TX.14 and RX.15 in ALT0 mode)
    mov w2, #0x20000
    orr w2, w2, #0x04000
    str w2, [x1, #0x4]

    // GPIO->PUP_PDN_CNTRL_REG0 &= ~0xf0000000 (no resistor for TX and RX pins)
    ldr w2, [x1, #0xe4]
    bic w2, w2, #0xf0000000
    //movk w3, #0xa000, lsl #16
    //orr w2, w2, w3
    str w2, [x1, #0xe4]

    str wzr, [x0, #0x30] // UART->CR = 0

    mov w2, #0x7ff
    str w2, [x0, #0x44] // UART->ICR = 0x7ff

    // Divider = 3.2552083
    mov w2, #3
    str w2, [x0, #0x24] // UART->IBRD = 3
    mov w2, #16
    str w2, [x0, #0x28] // UART->FBRD = 16

    mov w2, #0x70
    str w2, [x0, #0x2c] // UART->LCRH = 0x70

    mov w2, #0x300
    str w2, [x0, #0x30] // UART->CR = 0x300

    mov w2, #0x301
    str w2, [x0, #0x30] // UART->CR = 0x301

    mov sp, #0x7ff0
    ldr x0, =el_text
    bl puts_nonewline
    mrs x0, CurrentEL
    bl put_hex32
    mov w0, #13
    bl putc
    mov w0, #10
    bl putc
done:
    b done
el_text:
    .asciz "CurrentEL: "

.align 16
putc:
    ldr x1, =0xfe201000 // UART_BASE
_putc_wait:
    dsb sy
    ldr w2, [x1, #0x18] // w2 = UART->FR
    ands w2, w2, #0x20
    b.ne _putc_wait
    str w0, [x1] // UART->DR = x0
    dsb sy
    ret

.align 16
puts_nonewline:
    stp x19, lr, [sp, #-0x10]!
    mov x19, x0
    mov x20, lr
_puts_begin:
    ldrb w0, [x19], #1
    cmp w0, #0
    b.eq _puts_end
    bl putc
    b _puts_begin
_puts_end:
    ldp x19, lr, [sp], #0x10
    ret

.align 16
putc_hex32:
    stp lr, xzr, [sp, #-0x10]!
    and w0, w0, #0xf
    cmp w0, #10
    b.gt _putc_hex32_hex
    add w0, w0, #48
    b _putc_hex32_end
_putc_hex32_hex:
    add w0, w0, #87
_putc_hex32_end:
    bl putc
    ldp lr, xzr, [sp], #0x10
    ret

.align 16
put_hex32:
    stp x19, x20, [sp, #-0x10]!
    stp lr, xzr, [sp, #-0x10]!
    mov x19, x0
    mov x20, #28
_put_hex32_begin:
    lsr x0, x19, x20
    bl putc_hex32
    cmp x20, #0
    b.eq _put_hex32_end
    sub x20, x20, #4
    b _put_hex32_begin
_put_hex32_end:
    ldp lr, xzr, [sp], #0x10
    ldp x19, x20, [sp], #0x10
    ret

/*
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
    // First, run clock_max_out_arm on a valid stack
    ldr sp, =core0stack     //  set sp of core 0 (SVC)
    bl clock_max_out_arm

    mov r4, #0x40000000

    ldr r5, =start_core1
    // Comment out to "disable" core 1
    str r5, [r4, #0x9c]

    ldr r5, =start_core2
    // Comment out to "disable" core 2
    str r5, [r4, #0xac]

    ldr r5, =start_core3
    // Comment out to "disable" core 3
    str r5, [r4, #0xbc]

    // This portion of code is only run by core 0
    sev // Signal event to the other cores to ensure  they are awake
    bl enable_svc_mode
    bl init_irq
    ldr sp, =core0stack     //  set sp of core 0 (SVC)
    bl main0

done:
    b done

start_core1:
    bl enable_svc_mode
    bl init_irq
    ldr sp, =core1stack     //  set sp of core 1 (SVC)
    bl main1
    b done

start_core2:
    bl enable_svc_mode
    bl init_irq
    ldr sp, =core2stack     //  set sp of core 2 (SVC)
    bl main2
    b done

start_core3:
    bl enable_svc_mode
    bl init_irq
    ldr sp, =core3stack     //  set sp of core 3 (SVC)
    bl main3
b done
*/

.section .stack
.align 16
.space 0x800-4
irqstack: .space 4
.space 0x800-4
core0stack: .space 4
.space 0x800-4
core1stack: .space 4
.space 0x800-4
core2stack: .space 4
.space 0x800-4
core3stack: .space 4
