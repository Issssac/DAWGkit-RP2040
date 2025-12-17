#include "hardware/regs/addressmap.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/sio.h"
#include "hardware/regs/watchdog.h"
#include "hardware/regs/syscfg.h"
#include "hardware/regs/clocks.h"
#include "hardware/regs/vreg_and_chip_reset.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/resets.h"
.cpu cortex-m0
.thumb

.align 2
.global fake_reset_block_noinline
.type fake_reset_block_noinline,%function
.thumb_func
fake_reset_block_noinline:
    ldr r1, =RESETS_BASE + REG_ALIAS_SET_BITS
    str r0, [r1]
    bx lr

.global fake_reset_unreset_block_wait_noinline
.type fake_reset_unreset_block_wait_noinline,%function
.thumb_func
fake_reset_unreset_block_wait_noinline:
    ldr r1, =RESETS_BASE + REG_ALIAS_SET_BITS
    str r0, [r1]
    // fall thru

.global fake_unreset_block_wait_noinline
.type fake_unreset_block_wait_noinline,%function
.thumb_func
fake_unreset_block_wait_noinline:
    ldr r1, =RESETS_BASE
    ldr r2, =RESETS_BASE + REG_ALIAS_CLR_BITS
    str r0, [r2]
1:
    ldr r3, =(RESETS_RESET_DONE_OFFSET)
    ldr r2, [r1, r3]
    bic r0, r2
    bne 1b
    bx lr
