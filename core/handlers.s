.section .text
.global start
start:
    ;@ Assumes that processor is in AArch32 mode, i.e., arm_core & 0x200 == 0

    ;@ Zero bss section
    ldr r0, =__bss_begin
    ldr r1, =__bss_end
    mov r2, #0
bssloop:
    cmp r0, r1
    beq launch
    str r2, [r0], #4
    b bssloop
    
launch:
    ;@ Set sp of IRQ mode
    cps #0x12
    ldr sp, #irqstack
    cps #0x13 ;@ Switch to supervisor mode

    mov r4, #0x40000000

    ldr r5, =start_core1
    str r5, [r4, #0x9c]

    ldr r5, =start_core2
    str r5, [r4, #0xac]

    ldr r5, =start_core3
    str r5, [r4, #0xbc]

    ldr sp, #core0stack     ;@  set sp of core 0
    bl main0

done:
    b done

start_core1:
    ldr sp, #core1stack     ;@  set sp of core 1
    bl main1
    b done

start_core2:
    ldr sp, #core2stack     ;@  set sp of core 2
    bl main2
    b done

start_core3:
    ldr sp, #core3stack     ;@  set sp of core 3
    bl main3
    b done

core0stack: .word 0x00008000
core1stack: .word 0x00007c00
core2stack: .word 0x00007800
core3stack: .word 0x00007400

irqstack  : .word 0x00007000

.global UndefinedHandler
UndefinedHandler:
;@ b UndefinedHandler
subs pc, lr, #4

.global SwiHandler
SwiHandler:
;@ b SwiHandler
subs pc, lr, #4

.global PrefetchHandler
PrefetchHandler:
;@ b PrefetchHandler
subs pc, lr, #4

.global DataHandler
DataHandler:
;@ b DataHandler
subs pc, lr, #4

.global UnusedHandler
UnusedHandler:
;@ b UnusedHandler
subs pc, lr, #4

;@.global IRQHandler
;@IRQHandler:       b IRQHandler

.global FIQHandler
FIQHandler:
;@ b FIQHandler
subs pc, lr, #4
