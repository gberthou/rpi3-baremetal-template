.section .text.startup
	ldr pc, resetHandler
	ldr pc, undefinedHandler
	ldr pc, swiHandler
	ldr pc, prefetchHandler
	ldr pc, dataHandler
	ldr pc, unusedHandler
	ldr pc, irqHandler
	ldr pc, fiqHandler

.global _start
_start:
resetHandler:     .word ResetHandler
undefinedHandler: .word UndefinedHandler
swiHandler:       .word SwiHandler
prefetchHandler:  .word PrefetchHandler
dataHandler:      .word DataHandler
unusedHandler:    .word UnusedHandler
irqHandler:       .word IRQHandler
fiqHandler:       .word FIQHandler
