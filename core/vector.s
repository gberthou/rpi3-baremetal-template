.section .text.startup
.global _start
_start:
	b ResetHandler
	b UndefinedHandler
	b SwiHandler
	b PrefetchHandler
	b DataHandler
	b UnusedHandler
	b IRQHandler
	b FIQHandler
