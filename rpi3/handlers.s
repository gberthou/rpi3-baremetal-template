.section .text
.global IRQHandler
IRQHandler:
    subs pc, lr, #4

.global UndefinedHandler
UndefinedHandler:
    subs pc, lr, #4

.global SwiHandler
SwiHandler:
    subs pc, lr, #4

.global PrefetchHandler
PrefetchHandler:
    subs pc, lr, #4

.global DataHandler
DataHandler:
    subs pc, lr, #4

.global UnusedHandler
UnusedHandler:
    subs pc, lr, #4

.global FIQHandler
FIQHandler:
    subs pc, lr, #4
