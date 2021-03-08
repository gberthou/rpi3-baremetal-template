.global _start
_start:
/*
    r0~r4 are passed to the kmain
    r5 = stack value
*/
mov sp, r5
bl kmain
rts
