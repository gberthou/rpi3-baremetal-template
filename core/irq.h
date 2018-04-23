#ifndef IRQ_H
#define IRQ_H

#if 0
#define IRQ_ENABLE()  __asm__ volatile("push {r0}\n"\
                                       "mrs r0, cpsr\n"\
                                       "bic r0, r0, #0x80\n"\
                                       "msr cpsr, r0\n"\
                                       "pop {r0}\n")

#define IRQ_DISABLE()  __asm__ volatile("push {r0}\n"\
                                        "mrs r0, cpsr\n"\
                                        "orr r0, r0, #0x80\n"\
                                        "msr cpsr, r0\n"\
                                        "pop {r0}\n")
#else
#define IRQ_ENABLE() __asm__ __volatile__("cpsie i")
#define IRQ_DISABLE() __asm__ __volatile__("cpsid i")
#endif

#endif

