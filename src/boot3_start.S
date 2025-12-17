#include "hardware/regs/m0plus.h"

.cpu cortex-m0
.thumb

.section .boot3, "ax"
.balign 2

.global boot3_start
boot3_start:
    ldr r0, =(0x10ffd018)
    ldr r1, =(0xe0000000 + 0x0000ed08)
    str r0, [r1]
    ldmia r0, {r0, r1}
    msr msp, r0
    bx r1