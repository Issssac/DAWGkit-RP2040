#include "hardware/regs/m0plus.h"
#include "pico/asm_helper.S"

pico_default_asm_setup

.section .boot3, "ax"
.global boot3_start
boot3_start:
    nop
    nop
    ldr r0, =(0x10ff9100)
    ldr r1, =(PPB_BASE + M0PLUS_VTOR_OFFSET)
    str r0, [r1]
    ldmia r0, {r0, r1}
    msr msp, r0
    bx r1