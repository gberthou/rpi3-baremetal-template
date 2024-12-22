// This is a small standalone program that prints CurrentEL: <value>\r\n over
// UART at 921600 baud. This program can be useful in order to test one's
// config.txt file without creating a proper runtime environment for C code. A
// minimal UART driver is embedded here in order for this program to be
// standalone.

.section .text

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
