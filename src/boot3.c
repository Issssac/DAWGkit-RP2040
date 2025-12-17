#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "bootrom/bootrom_main.c"

extern int _wd_boot_main();

#define BOOTROM_PTR 0x000024d1
#define VTOR_PTR 0x10000100

#define WD_SR_4 0x4005801c
#define WD_SR_5 0x40058020
#define WD_SR_6 0x40058024
#define WD_SR_7 0x40058028
#define WD_MAGIC_NUMBER 0xb007c0d3
#define ENTRY_PTR BOOT3_VTOR
#define SP_WD 0x21000000
#define WD_LOADED() (*((volatile uint32_t*)WD_SR_4)==WD_MAGIC_NUMBER)

static inline void WD_SET_SR() {
    *((volatile uint32_t*)WD_SR_4) = WD_MAGIC_NUMBER;
    *((volatile uint32_t*)WD_SR_5) = (-WD_MAGIC_NUMBER) ^ ENTRY_PTR;
    *((volatile uint32_t*)WD_SR_6) = SP_WD;
    *((volatile uint32_t*)WD_SR_7) = ENTRY_PTR;
}

static inline void PAYLOAD() {
    //TODO: Toggle GPIO 25
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);
    gpio_xor_mask(0b1 << 25);
}

static void watchdogHandler(){
    //Reset WD Scratch
    WD_SET_SR();

    //Check if Boot2 needs to be modified
    _wd_boot_main();
    
    //Run bootrom main
    asm volatile("bx %0": :"r" (BOOTROM_PTR));
}

int main(){
    if (!WD_LOADED()) {
        watchdogHandler();
    }
    else {
        //Payload execution
        PAYLOAD();

        //Run user application as if nothing happened
        uint32_t MSP = *(volatile uint32_t*)VTOR_PTR;
        uint32_t MAIN = *(((volatile uint32_t*)VTOR_PTR)+1);
        *((volatile uint32_t*)(PPB_BASE + M0PLUS_VTOR_OFFSET))=VTOR_PTR;
        asm volatile(
            "msr msp, %0\n\t"
            "bx %1"
            : 
            :"r"(MSP), "r"(MAIN)
            :"memory"
        ); 
    }
}