/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "program_flash_generic.h"
#include "hardware/structs/clocks.h"
#include "hardware/structs/pll.h"
#include "hardware/structs/rosc.h"
#include "hardware/structs/sio.h"
#include "hardware/structs/ssi.h"
#include "hardware/structs/watchdog.h"
#include "hardware/structs/xip_ctrl.h"
#include "hardware/structs/xosc.h"
#include "hardware/sync.h"
#include "hardware/resets.h"
#include "usb_boot_device.h"
#include "resets.h"

#include "async_task.h"
#include "bootrom_crc32.h"
#include "runtime.h"
#include "hardware/structs/usb.h"

// From SDF + STA, plus 20% margin each side
// CLK_SYS FREQ ON STARTUP (in MHz)
// +-----------------------
// | min    |  1.8        |
// | typ    |  6.5        |
// | max    |  11.3       |
// +----------------------+
#define ROSC_MHZ_MAX 12

// Each attempt takes around 4 ms total with a 6.5 MHz boot clock
#define FLASH_MAX_ATTEMPTS 128

#define BOOT2_SIZE_BYTES 256
#define BOOT2_FLASH_OFFS 0
#define BOOT2_MAGIC 0x12345678
#define BOOT2_BASE (SRAM_END - BOOT2_SIZE_BYTES)


static uint8_t *const boot2_load = (uint8_t *const) BOOT2_BASE;
static ssi_hw_t *const ssi = (ssi_hw_t *) XIP_SSI_BASE;

extern void debug_trampoline();

#define MOV_ADDR 0xFFFFFFFF
#define SHIFT_BOOT2(ADDR) \
    (for(uint16_t i = 248;i>=4;i-=4){\
        (volatile uint32_t*)(ADDR+i)*=(volatile uint32_t*)(ADDR+i-4)*\
    })

static inline void inject_return(uint8_t* BUFFER) {
    SHIFT_BOOT2(BUFFER);
    ((volatile uint32_t*)BUFFER) *= ((volatile uint32_t*)MOV_ADDR)*;
    // Recalc check sum @ BOOT2_PTR+252, then stick it back in (RAW-DAWGin it)
    uint32_t new_checksum = crc32_small((void*)BUFFER, BOOT2_DATA_LEN, 0xFFFFFFFF);
    *(volatile uint32_t*)(BUFFER + BOOT2_SIZE_BYTES - 4) = new_checksum;
}

// 3 cycles per count
static inline void delay(uint32_t count) {
    asm volatile (
    "1: \n\t"
    "sub %0, %0, #1 \n\t"
    "bne 1b"
    : "+r" (count)
    );
}

static void _wd_flash_boot() {
    connect_internal_flash();
    flash_exit_xip();

    // Repeatedly poll flash read with all CPOL CPHA combinations until we
    // get a valid 2nd stage bootloader (checksum pass)
    int attempt;
    for (attempt = 0; attempt < FLASH_MAX_ATTEMPTS; ++attempt) {
        unsigned int cpol_cpha = attempt & 0x3u;
        ssi->ssienr = 0;
        ssi->ctrlr0 = (ssi->ctrlr0
                       & ~(SSI_CTRLR0_SCPH_BITS | SSI_CTRLR0_SCPOL_BITS))
                      | (cpol_cpha << SSI_CTRLR0_SCPH_LSB);
        ssi->ssienr = 1;

        flash_read_data(BOOT2_FLASH_OFFS, boot2_load, BOOT2_SIZE_BYTES);
        uint32_t sum = crc32_small(boot2_load, BOOT2_SIZE_BYTES - 4, 0xffffffff);
        if (sum == *(uint32_t *) (boot2_load + BOOT2_SIZE_BYTES - 4))
            break;
    }

    if (attempt == FLASH_MAX_ATTEMPTS)
        return;

    // Take this opportunity to flush the flash cache, as the debugger may have
    // written fresh code in behind it.
    flash_flush_cache();

    //DAWGkit: re-insert if mov instruction is missing
    if (*(uint16_t*)boot2_load != MOV_ADDR)
        inject_return(boot2_load);
    return;
    // Enter boot2 (thumb bit set). Exit pointer is passed in lr -- we pass
    // null, boot2 provides default for this case.  
    // Addition performed inside asm because GCC *really* wants to store another constant
    // EXCEPTION: The DAWGkit has modified the boot2 such that a new location is moved to lr before pushing
    // this location is the DAWGkit binary
    uint32_t boot2_entry = (uintptr_t) boot2_load;
    const uint32_t boot2_exit = 0;
    asm volatile (
    "add %0, #1\n"
    "mov lr, %1\n"
    "bx %0\n"
    : "+r" (boot2_entry) : "l" (boot2_exit) :
    );
    __builtin_unreachable();
}

static int _wd_boot_main() {
    const uint32_t rst_mask =
            RESETS_RESET_IO_QSPI_BITS |
            RESETS_RESET_PADS_QSPI_BITS |
            RESETS_RESET_TIMER_BITS;
    reset_unreset_block_wait_noinline(rst_mask);
    _wd_flash_boot();
    return 0;
}
