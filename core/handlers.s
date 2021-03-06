.section .text

enable_svc_mode:
    cpsid if
    ;@ 1. check if core in HYP mode
    mrs r0, cpsr      ;@ r0 = cpsr
    and r1, r0, #0x1f ;@ r1 = mode bits of cpsr (0-4)
    mov r2, #0x1a
    cmp r1, r2        ;@ if mode bits != 0x1a (HYP mode)
    bxne lr           ;@ Return = jump to normal boot without caring about the
                      ;@ mode
    ;@ 2. return from HYP mode
    msr ELR_hyp, lr   ;@ After eret instruction, pc = return address
    bic r1, r0, #0x1f ;@ r1 = cpsr with mode 0x00
    orr r1, r1, #0x13 ;@ r1 = cpsr with mode 0x13 (SVC)
    msr SPSR_hyp, r1  ;@ After eret instruction, cpsr = r1 = old cpsr with mode
                      ;@ SVC
    eret ;@ Return from HYP exception

;@ Assumes that the caller is in SVC mode (lr register is banked)
init_irq:
    cps #0x12         ;@ Switch to IRQ mode
    ldr sp, #irqstack ;@ Set IRQ stack
    cps #0x13         ;@ Switch to SVC mode

    ;@ Init interrupt vector base address
    mov r0, #0x8000
    mcr p15, 0, r0, c12, c0, 0 ;@ VBAR
    
    bx lr ;@ Return

.global start
start:
    ;@ Assumes that processor is in AArch32 mode, i.e., arm_core & 0x200 == 0

    ;@ /!\ RPI 2/3's firmware puts arm core in HYP mode (cps #0x1a) which has
    ;@ another vector than the other arm modes.

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
    ;@ First, run clock_max_out_arm on a valid stack
    ldr sp, #core0stack     ;@  set sp of core 0 (SVC)
    bl clock_max_out_arm

    mov r4, #0x40000000

    ldr r5, =start_core1
    ;@ Comment out to "disable" core 1
    str r5, [r4, #0x9c]

    ldr r5, =start_core2
    ;@ Comment out to "disable" core 2
    str r5, [r4, #0xac]

    ldr r5, =start_core3
    ;@ Comment out to "disable" core 3
    str r5, [r4, #0xbc]

    ;@ This portion of code is only run by core 0
    sev ;@ Signal event to the other cores to ensure  they are awake
    bl enable_svc_mode
    bl init_irq
    ldr sp, #core0stack     ;@  set sp of core 0 (SVC)
    bl main0

done:
    b done

start_core1:
    bl enable_svc_mode
    bl init_irq
    ldr sp, #core1stack     ;@  set sp of core 1 (SVC)
    bl main1
    b done

start_core2:
    bl enable_svc_mode
    bl init_irq
    ldr sp, #core2stack     ;@  set sp of core 2 (SVC)
    bl main2
    b done

start_core3:
    bl enable_svc_mode
    bl init_irq
    ldr sp, #core3stack     ;@  set sp of core 3 (SVC)
    bl main3
    b done

core0stack: .word 0x00008000
core1stack: .word 0x00007c00
core2stack: .word 0x00007800
core3stack: .word 0x00007400

irqstack: .word 0x00007000

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

;@ Implemented in `app/main.c`
;@.global IRQHandler
;@IRQHandler:
;@subs pc, lr, #4

.global FIQHandler
FIQHandler:
;@ b FIQHandler
subs pc, lr, #4
