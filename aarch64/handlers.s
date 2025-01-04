.section .text
.global IRQHandler
IRQHandler:
    eret

.global UndefinedHandler
UndefinedHandler:
    eret

.global SwiHandler
SwiHandler:
    eret

.global PrefetchHandler
PrefetchHandler:
    eret

.global DataHandler
DataHandler:
    eret

.global UnusedHandler
UnusedHandler:
    eret

.global FIQHandler
FIQHandler:
    eret
